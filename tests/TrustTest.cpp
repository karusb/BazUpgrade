#include "storage/MetadataDirectory.hpp"
#include "tests/TestSupport.hpp"

namespace test
{
class TrustTest : public RepositoryTest
{
};

TEST(TrustValidationTest, CanonicalJSONSortsKeysAndPreservesUTF8)
{
    const auto value = ParseJson(R"({"z":1,"a":"line\nnext","b":true})");

    EXPECT_EQ(std::string("{\"a\":\"line\nnext\",\"b\":true,\"z\":1}"), CanonicalJson(value));

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [] { CanonicalJson(Json(1.5)); }));

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [] { ParseJson(R"({"a":1,"a":2})"); }));
}

TEST(TrustValidationTest, Ed25519KnownVectorRejectsChangedMessages)
{
    OpenSslCrypto crypto;
    Json key = {{"keytype", "ed25519"},
                {"scheme", "ed25519"},
                {"keyval", {{"public", "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"}}}};
    std::string signature = "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821"
                            "590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b";

    ASSERT_TRUE(crypto.Verify(key, "", signature)) << "RFC 8032 vector rejected";

    EXPECT_FALSE(crypto.Verify(key, "changed", signature)) << "Changed message accepted";
}

TEST_F(TrustTest, SignedPublicationResolvesAndDownloads)
{
    repository.PublishApplicationRelease();

    DirectoryTransport transport(repository.repo);
    TufClient client(transport, repository.crypto, repository.bootstrap, repository.root / "trust");

    auto target = client.ResolveTarget(repository.config.target, repository.cancel);
    client.DownloadAndVerifyTarget(target, repository.root / "download.zip", repository.cancel);

    EXPECT_EQ(1, target.custom.at("releaseSequence"));

    EXPECT_EQ(repository.crypto.HashFile(repository.package),
              repository.crypto.HashFile(repository.root / "download.zip"));
}

TEST_F(TrustTest, ThresholdSignaturesCountUniqueAuthorizedKeys)
{
    auto envelope = ReadJson(repository.bootstrap);
    const auto body = envelope.at("signed");

    envelope["signatures"] = Json::array({envelope.at("signatures").at(0), envelope.at("signatures").at(0)});

    ASSERT_TRUE(FailsWithUpdateError(
        ErrorCode::trust,
        [&] { VerifyThreshold(repository.crypto, envelope, body.at("keys"), body.at("roles").at("root")); }));
}

TEST_F(TrustTest, ChangedSignedBodyFailsVerification)
{
    auto envelope = ReadJson(repository.bootstrap);
    const auto authority = envelope.at("signed");

    envelope["signed"]["expires"] = "2099-01-01T00:00:00Z";

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust,
                                     [&] {
                                         VerifyThreshold(repository.crypto, envelope, authority.at("keys"),
                                                         authority.at("roles").at("root"));
                                     }));
}

TEST_F(TrustTest, ExpiredTimestampFailsClosed)
{
    repository.PublishApplicationRelease();

    auto timestamp = ReadJson(repository.repo / "metadata/timestamp.json").at("signed");

    timestamp["expires"] = "2000-01-01T00:00:00Z";
    repository.WriteSignedMetadata("timestamp.json", timestamp, "timestamp");
    UpdateClient client(repository.config);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::expired, [&] { client.CheckForUpdate(repository.cancel); }));
}

TEST_F(TrustTest, MetadataVersionRollbackSurvivesANewClient)
{
    repository.PublishApplicationRelease();

    auto old = ReadJson(repository.repo / "metadata/timestamp.json");

    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
    repository.PublishApplicationRelease(2);

    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
    WriteJson(repository.repo / "metadata/timestamp.json", old);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::rollback, [&]
                                     { UpdateClient(repository.config).CheckForUpdate(repository.cancel); }));
}

TEST_F(TrustTest, MetadataCannotChangeAtTheSameVersion)
{
    repository.PublishApplicationRelease();

    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
    auto timestamp = ReadJson(repository.repo / "metadata/timestamp.json").at("signed");

    timestamp["expires"] = "2099-01-01T00:00:00Z";
    repository.WriteSignedMetadata("timestamp.json", timestamp, "timestamp");

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&]
                                     { UpdateClient(repository.config).CheckForUpdate(repository.cancel); }));
}

TEST_F(TrustTest, RootRotationWorksFromTheOriginalBootstrap)
{
    repository.PublishApplicationRelease();

    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
    RepositoryPublisher(repository.repo, repository.keys).RotateRoot();

    ASSERT_EQ("updateAvailable",
              UpdateClient(repository.config).CheckForUpdate(repository.cancel).at("status"))
        << "Rotation lost target";

    ASSERT_EQ(2, ReadJson(repository.config.stateDirectory / "trust/root.json").at("signed").at("version"))
        << "New root was not persisted";
}

TEST_F(TrustTest, CorruptedArtifactIsRejectedBeforeExtraction)
{
    repository.PublishApplicationRelease();

    auto target = ReadJson(repository.repo / "metadata/targets.json")
                      .at("signed")
                      .at("targets")
                      .at(repository.config.target);
    auto path = repository.repo / "targets" /
                (target.at("hashes").at("sha256").get<std::string>() + "." + repository.config.target);
    auto bytes = ReadFile(path);

    bytes[0] ^= 1;
    WriteFixtureFile(path, bytes);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&]
                                     { UpdateClient(repository.config).ApplyUpdate(repository.cancel); }));

    ASSERT_FALSE(fs::exists(repository.config.installDirectory / "versions"))
        << "Unverified archive was extracted";
}

TEST_F(TrustTest, MissingTargetIsAControlledError)
{

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&]
                                     { UpdateClient(repository.config).CheckForUpdate(repository.cancel); }));
}

TEST_F(TrustTest, DelegatedRoleCacheCannotAliasTheTrustedRoot)
{
    repository.PublishApplicationRelease();

    repository.AddDelegatedRole("ROOT", "ex[a-z]mple.?ip", false, true);
    auto original = ReadJson(repository.bootstrap);

    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
    auto trust = repository.config.stateDirectory / "trust";

    ASSERT_EQ(original, ReadJson(trust / "root.json")) << "Delegation replaced trust root";

    ASSERT_TRUE(fs::exists(trust / "delegated" / (repository.crypto.Hash("ROOT") + ".json")))
        << "Delegated cache is not isolated";
    UpdateClient(repository.config).CheckForUpdate(repository.cancel);
}

TEST_F(TrustTest, TerminatingBracketDelegationStopsLaterRoles)
{
    repository.PublishApplicationRelease();

    repository.AddDelegatedRole("first", "ex[!0-9]mple.*", true, false);
    repository.AddDelegatedRole("later", "*", false, false);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&]
                                     { UpdateClient(repository.config).CheckForUpdate(repository.cancel); }));

    ASSERT_FALSE(fs::exists(repository.config.stateDirectory / "trust/delegated" /
                            (repository.crypto.Hash("later") + ".json")))
        << "Terminated traversal visited a later role";
}

TEST_F(TrustTest, PublisherKeepsKeysOutsideTheWebRoot)
{

    ASSERT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&]
                             { RepositoryPublisher invalid(repository.repo, repository.repo / "keys"); }));
}

TEST_F(TrustTest, PublisherRejectsReplayedMetadataAndRestoresProtectedPublication)
{
    repository.PublishApplicationRelease();

    std::vector<Json> old;

    for (const std::string role : {"targets", "snapshot", "timestamp"})
        old.push_back(ReadJson(repository.repo / "metadata" / (role + ".json")));

    repository.PublishApplicationRelease(2);

    auto checkpoint = ReadJson(repository.keys / "publication.json");
    unsigned index = 0;

    for (const std::string role : {"targets", "snapshot", "timestamp"})
        WriteJson(repository.repo / "metadata" / (role + ".json"), old.at(index++));

    for (unsigned attempt = 0; attempt < 4; ++attempt)
        ASSERT_TRUE(FailsWithUpdateError(
            ErrorCode::rollback, [&] { RepositoryPublisher(repository.repo, repository.keys).Refresh(); }));

    ASSERT_EQ(checkpoint, ReadJson(repository.keys / "publication.json"))
        << "Rejected replay changed signing state";

    RepositoryPublisher(repository.repo, repository.keys).RestoreMetadata();

    ASSERT_EQ(2, UpdateClient(repository.config).CheckForUpdate(repository.cancel).at("releaseSequence"))
        << "Restoration lost the latest release";
}

#ifdef _WIN32
TEST_F(TrustTest, PublisherRejectsWindowsCaseAliasesInsideItsServedRoot)
{
    auto alias = repository.repo.parent_path() / "REPOSITORY" / "private-nested";

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::invalidInput,
                                     [&] { RepositoryPublisher invalid(repository.repo, alias); }));

    ASSERT_FALSE(fs::exists(alias)) << "Rejected key directory was created";
}

#endif
TEST(TrustValidationTest, PublisherRefusesIndependentSnapshotAndTimestampReplays)
{
    for (const std::string role : {"snapshot", "timestamp"})
    {
        SCOPED_TRACE(role);
        RepositoryFixture repository;

        repository.PublishApplicationRelease();
        auto path = repository.repo / "metadata" / (role + ".json");
        auto previous = ReadJson(path);

        repository.PublishApplicationRelease(2);
        auto protectedState = ReadJson(repository.keys / "publication.json");

        WriteJson(path, previous);

        ASSERT_TRUE(FailsWithUpdateError(
            ErrorCode::rollback, [&] { RepositoryPublisher(repository.repo, repository.keys).Refresh(); }));
        ASSERT_EQ(protectedState, ReadJson(repository.keys / "publication.json"))
            << "Refused replay changed protected state";
    }
}

TEST_F(TrustTest, InterruptedPublicationRequiresExplicitExactRestoration)
{
    repository.PublishApplicationRelease();

    auto oldCheckpoint = ReadJson(repository.keys / "publication.json");
    auto next = oldCheckpoint.at("envelopes").at("snapshot").at("signed").at("version").get<unsigned>() + 1;
    auto obstruction = repository.repo / "metadata" / (std::to_string(next) + ".snapshot.json");

    fs::create_directory(obstruction);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::io, [&]
                                     { RepositoryPublisher(repository.repo, repository.keys).Refresh(); }));
    auto checkpoint = ReadJson(repository.keys / "publication.json");

    ASSERT_NE(oldCheckpoint, checkpoint) << "Interrupted publication lost protected intent";

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::rollback, [&]
                                     { RepositoryPublisher(repository.repo, repository.keys).Refresh(); }));
    fs::remove(obstruction);
    RepositoryPublisher(repository.repo, repository.keys).RestoreMetadata();
    for (const std::string role : {"targets", "snapshot", "timestamp"})
        ASSERT_EQ(checkpoint.at("envelopes").at(role),
                  ReadJson(repository.repo / "metadata" / (role + ".json")))
            << "Restore changed signed bytes";
    RepositoryPublisher(repository.repo, repository.keys).Refresh();

    ASSERT_EQ(checkpoint.at("envelopes").at("timestamp").at("signed").at("version").get<unsigned>() + 1,
              ReadJson(repository.keys / "publication.json")
                  .at("envelopes")
                  .at("timestamp")
                  .at("signed")
                  .at("version"))
        << "Refresh did not advance restored state";
}

#ifdef _WIN32
TEST_F(TrustTest, MetadataDirectoryCannotBeReplacedDuringATransaction)
{
    MetadataDirectory directory(repository.repo);
    std::error_code error;

    fs::rename(repository.repo / "metadata", repository.repo / "detached-metadata", error);

    ASSERT_TRUE(static_cast<bool>(error)) << "Directory replacement bypassed the transaction anchor";
    directory.Write("anchor.json", {{"value", "synthetic"}});

    ASSERT_TRUE(fs::exists(repository.repo / "metadata/anchor.json")) << "Anchored write failed";
}

TEST_F(TrustTest, MetadataHandlesRejectWindowsJunctionRedirection)
{
    auto outside = repository.root / "outside";
    fs::create_directory(outside);
    MetadataDirectory directory(repository.repo);
    repository.ReplaceMetadataWithJunctionTo(outside);

    try
    {
        directory.Write("anchor.json", {{"value", "synthetic"}});
    }
    catch (const Error &error)
    {
        ASSERT_EQ(ErrorCode::io, error.code) << "Unexpected junction rejection";
    }

    ASSERT_FALSE(fs::exists(outside / "anchor.json")) << "Metadata write followed a changed junction";
}

#else
TEST_F(TrustTest, MetadataWritesRemainAnchoredAcrossDirectoryReplacement)
{
    auto outside = repository.root / "outside";
    fs::create_directory(outside);
    MetadataDirectory directory(repository.repo);
    fs::rename(repository.repo / "metadata", repository.repo / "detached-metadata");
    fs::create_directory_symlink(outside, repository.repo / "metadata");
    directory.Write("anchor.json", {{"value", "synthetic"}});

    ASSERT_FALSE(fs::exists(outside / "anchor.json")) << "Write followed a replaced directory";

    ASSERT_TRUE(fs::exists(repository.repo / "detached-metadata/anchor.json"))
        << "Write lost its original directory anchor";

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::io, [&] { MetadataDirectory redirected(repository.repo); }));
}

#endif
TEST(TrustValidationTest, CalendarDatesAndPathVariantsAreValidated)
{

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::trust, [] { ParseUtc("2026-02-30T00:00:00Z"); }));
    for (const std::string path :
         {"../escape", "/absolute", "C:/drive", "a\\\\b", "a%2fb", "CON.txt", "a//b", "a/../b", "a."})
        ASSERT_TRUE(FailsWithUpdateError(ErrorCode::invalidInput, [&] { SafeRelative(path); }));
}

struct DelegationPatternCase
{
    const char *name;
    const char *pattern;
    const char *target;
    bool matches;
};

class DelegationPatternTest : public RepositoryTest,
                              public ::testing::WithParamInterface<DelegationPatternCase>
{
};

TEST_P(DelegationPatternTest, AuthorizesOnlyMatchingTargets)
{
    const auto &scenario = GetParam();

    repository.config.target = scenario.target;
    repository.PublishApplicationRelease();
    repository.AddDelegatedRole("pattern", scenario.pattern, false, true);
    UpdateClient client(repository.config);

    if (scenario.matches)
    {
        EXPECT_EQ("updateAvailable", client.CheckForUpdate(repository.cancel).at("status"));
        EXPECT_EQ("updated", client.ApplyUpdate(repository.cancel).at("status"));
    }
    else
        EXPECT_TRUE(
            FailsWithUpdateError(ErrorCode::trust, [&] { client.CheckForUpdate(repository.cancel); }));
}

INSTANTIATE_TEST_SUITE_P(
    DelegatedPaths, DelegationPatternTest,
    ::testing::Values(DelegationPatternCase{"WildcardSuffix", "example.*", "example.zip", true},
                      DelegationPatternCase{"SingleCharacter", "ex?mple.zip", "example.zip", true},
                      DelegationPatternCase{"InclusiveRange", "ex[a-z]mple.?ip", "example.zip", true},
                      DelegationPatternCase{"NegatedRange", "ex[!0-9]mple.*", "example.zip", true},
                      DelegationPatternCase{"ExcludedRange", "ex[0-9]mple.*", "example.zip", false},
                      DelegationPatternCase{"StarBacktracking", "*ample.?ip", "example.zip", true},
                      DelegationPatternCase{"UnicodePrefix", "channel/\xCE\xBB*", "channel/\xCE\xBB.zip",
                                            true},
                      DelegationPatternCase{"UnclosedBracketLiteral", "file[.zip", "file[.zip", true}),
    [](const ::testing::TestParamInfo<DelegationPatternCase> &info) { return info.param.name; });

TEST_F(TrustTest, ReadsUtf8FilesystemNamesWithoutDependingOnTheWindowsCodePage)
{
    const auto path = repository.root / fs::u8path("\xCE\xBB.txt");

    WriteFixtureFile(path, "synthetic UTF-8 file");

    EXPECT_EQ("synthetic UTF-8 file", ReadFile(path));
}
} // namespace test
