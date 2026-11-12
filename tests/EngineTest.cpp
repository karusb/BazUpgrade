#include "tests/TestSupport.hpp"
#include <functional>

namespace test
{
class RecordingInstaller final : public Installer
{
  public:
    explicit RecordingInstaller(std::function<InstallResult()> install) : install(std::move(install))
    {
    }

    InstallResult Apply(const Target &, const fs::path &, const fs::path &, const Cancellation &) override
    {
        ++calls;

        return install();
    }

    unsigned calls = 0;

  private:
    std::function<InstallResult()> install;
};

class EngineTest : public RepositoryTest
{
  protected:
    UpdateClient CreateClientWithInstaller(std::unique_ptr<Installer> installer)
    {
        return UpdateClient(repository.config, std::make_unique<OpenSslCrypto>(),
                            std::make_unique<DirectoryTransport>(repository.repo), std::move(installer));
    }
};

TEST(EngineValidationTest, HttpDevelopmentTransportRemainsLoopbackOnly)
{
    CurlTransport secure("https://example.invalid/repository");
    CurlTransport development("http://127.0.0.1:12345/repository", true);
    CurlTransport ipv6("http://[::1]:12345/repository", true);

    for (const auto &url :
         {"http://127.0.0.1/", "http://example.invalid/", "https://user:synthetic@example.invalid/"})
    {
        SCOPED_TRACE(url);
        ASSERT_TRUE(FailsWithUpdateError(ErrorCode::invalidInput, [&] { CurlTransport transport(url); }));
    }

    for (const auto &url :
         {"http://example.invalid/", "ftp://127.0.0.1/", "https://user:synthetic@example.invalid/",
          "https://@example.invalid/", "https://example.invalid/?", "https://example.invalid/#",
          "http://localhost.example.invalid/", "http://127.0.0.1.example.invalid/",
          "https://example.invalid:invalid/"})
    {
        SCOPED_TRACE(url);
        ASSERT_TRUE(
            FailsWithUpdateError(ErrorCode::invalidInput, [&] { CurlTransport transport(url, true); }));
    }
}

TEST_F(EngineTest, PreCancelledRepositoryTransfersCreateNoDownloadOutput)
{
    Cancellation cancel;
    CurlTransport remote("https://example.invalid/api/");
    DirectoryTransport local(repository.repo);
    const auto destination = repository.root / "cancelled-download.bin";

    cancel.requested = true;

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::cancelled,
                                     [&] { remote.Fetch("metadata/root.json", 1024, cancel); }));
    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::cancelled,
                                     [&] { remote.Download("artifact.zip", destination, 1024, cancel); }));
    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::cancelled,
                                     [&] { local.Download("artifact.zip", destination, 1024, cancel); }));

    EXPECT_FALSE(fs::exists(destination));
}

TEST_F(EngineTest, InvalidRepositoryPathsAreRejectedBeforeCreatingOutput)
{
    CurlTransport transport("https://example.invalid/api/");
    const auto destination = repository.root / "invalid-download.bin";

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::invalidInput,
                                     [&] { transport.Fetch("../metadata.json", 1024, repository.cancel); }));
    ASSERT_TRUE(FailsWithUpdateError(
        ErrorCode::invalidInput,
        [&] { transport.Download("../artifact.zip", destination, 1024, repository.cancel); }));

    EXPECT_FALSE(fs::exists(destination));
}

TEST_F(EngineTest, PortableUpdateActivatesAnImmutableVersion)
{
    repository.PublishApplicationRelease();
    UpdateClient client(repository.config);

    const auto result = client.ApplyUpdate(repository.cancel);
    const auto state = client.ReadInstallationState();
    const auto activePath = fs::u8path(state.at("activePath").get<std::string>());

    EXPECT_EQ("updated", result.at("status"));
    EXPECT_EQ("synthetic version one", ReadFile(activePath));
    EXPECT_EQ("upToDate", client.ApplyUpdate(repository.cancel).at("status"));
}

TEST_F(EngineTest, ReleaseSelectionUsesSequenceRatherThanLexicalVersion)
{
    auto descriptor = repository.BuildReleaseDescriptor(1);
    RepositoryPublisher publisher(repository.repo, repository.keys);

    descriptor["appVersion"] = "1.9.0";
    publisher.Publish(repository.package, repository.config.target, descriptor);
    UpdateClient(repository.config).ApplyUpdate(repository.cancel);

    repository.WriteApplicationArchive("synthetic version two");
    descriptor = repository.BuildReleaseDescriptor(2);
    descriptor["appVersion"] = "1.10.0";
    publisher.Publish(repository.package, repository.config.target, descriptor);

    const auto available = UpdateClient(repository.config).CheckForUpdate(repository.cancel);

    EXPECT_EQ("1.10.0", available.at("version"));
}

TEST_F(EngineTest, ASelectedTargetCannotCrossApplicationScope)
{
    repository.PublishApplicationRelease();
    repository.config.appId = "another-app";
    UpdateClient client(repository.config);

    EXPECT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&] { client.ApplyUpdate(repository.cancel); }));
}

TEST_F(EngineTest, StateCannotBeReusedWithAnotherChannel)
{
    repository.PublishApplicationRelease();
    UpdateClient(repository.config).CheckForUpdate(repository.cancel);

    repository.config.channel = "beta";
    UpdateClient anotherChannel(repository.config);

    EXPECT_TRUE(FailsWithUpdateError(ErrorCode::trust, [&] { anotherChannel.ReadInstallationState(); }));
}

TEST(EngineValidationTest, OperationLockDoesNotBlockUnrelatedApplications)
{
    RepositoryFixture first;
    RepositoryFixture second;

    first.PublishApplicationRelease();
    second.PublishApplicationRelease();

    FileLock active(first.config.stateDirectory / "operation.lock");
    UpdateClient blocked(first.config);
    UpdateClient independent(second.config);

    EXPECT_TRUE(FailsWithUpdateError(ErrorCode::busy, [&] { blocked.CheckForUpdate(first.cancel); }));
    EXPECT_EQ("updateAvailable", independent.CheckForUpdate(second.cancel).at("status"));
}

TEST_F(EngineTest, PreCancelledUpdateLeavesNoInstallerPending)
{
    repository.PublishApplicationRelease();
    repository.cancel.requested = true;
    UpdateClient client(repository.config);

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::cancelled, [&] { client.ApplyUpdate(repository.cancel); }));

    EXPECT_EQ("idle", client.ReadInstallationState().at("phase"));
}

TEST_F(EngineTest, UnknownOperationIsNeverAutomaticallyReplayed)
{
    repository.PublishApplicationRelease();
    UpdateClient client(repository.config);
    auto state = client.ReadInstallationState();

    state["phase"] = "applying";
    state["pending"] = {
        {"sequence", 1}, {"version", "1.1.0"}, {"sha256", repository.crypto.HashFile(repository.package)}};
    WriteJson(repository.config.stateDirectory / "installation.json", state);

    ASSERT_TRUE(
        FailsWithUpdateError(ErrorCode::unknownOutcome, [&] { client.ApplyUpdate(repository.cancel); }));

    EXPECT_EQ("idle", client.ReconcileInstallationOutcome(false).at("phase"));
}

TEST_F(EngineTest, ArchiveTraversalIsRejected)
{
    repository.CreateUnsafeArchive("../outside.txt");
    repository.PublishApplicationRelease();
    UpdateClient client(repository.config);

    ASSERT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ApplyUpdate(repository.cancel); }));

    EXPECT_FALSE(fs::exists(repository.config.installDirectory / "outside.txt"));
}

TEST_F(EngineTest, ArchiveSymlinksAreRejected)
{
    repository.CreateUnsafeArchive("app.txt", true);
    repository.PublishApplicationRelease();
    UpdateClient client(repository.config);

    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ApplyUpdate(repository.cancel); }));
}

TEST_F(EngineTest, ExtractionQuotaIsEnforcedIndependentlyOfCompressedSize)
{
    repository.PublishApplicationRelease();
    repository.config.maxExtractedBytes = 1;
    UpdateClient client(repository.config);

    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ApplyUpdate(repository.cancel); }));
}

TEST_F(EngineTest, DownloadQuotaIsEnforcedBeforeFetchingAnArtifact)
{
    repository.PublishApplicationRelease();
    repository.config.maxDownloadBytes = 1;
    UpdateClient client(repository.config);

    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ApplyUpdate(repository.cancel); }));
}

TEST_F(EngineTest, AnExternalUpdaterCanVerifyADownloadedArtifact)
{
    repository.PublishApplicationRelease();
    UpdateClient client(repository.config);

    const auto result = client.VerifyDownloadedFile(repository.package, repository.cancel);

    EXPECT_EQ("verified", result.at("status"));

    const auto wrongArtifact = repository.root / "wrong.zip";
    WriteFixtureFile(wrongArtifact, "wrong");

    EXPECT_TRUE(FailsWithUpdateError(ErrorCode::trust,
                                     [&] { client.VerifyDownloadedFile(wrongArtifact, repository.cancel); }));
}

TEST_F(EngineTest, PublisherRejectsSequenceReuse)
{
    repository.PublishApplicationRelease();

    EXPECT_TRUE(FailsWithUpdateError(ErrorCode::rollback, [&] { repository.PublishApplicationRelease(); }));
}

TEST(EngineValidationTest, CapabilitiesDistinguishNativeInstallationAndVerification)
{
    const auto result = Capabilities();

    EXPECT_FALSE(result.at("automaticElevation").get<bool>());
    EXPECT_FALSE(result.at("firmware").get<bool>());
    EXPECT_EQ("electron", result.at("verificationOnly").at(0));
}

TEST_F(EngineTest, CancellationAfterReadinessPreservesTheReadyPhase)
{
    repository.PublishApplicationRelease();
    auto installer = std::make_unique<RecordingInstaller>([] { return InstallResult{}; });
    auto &recording = *installer;
    auto client = CreateClientWithInstaller(std::move(installer));

    auto onReady = [&]
    {
        EXPECT_EQ("ready", ReadJson(repository.config.stateDirectory / "installation.json").at("phase"));
        repository.cancel.requested = true;
    };

    ASSERT_TRUE(FailsWithUpdateError(ErrorCode::cancelled,
                                     [&] { client.ApplyUpdate(repository.cancel, 0, onReady); }));

    EXPECT_EQ(0u, recording.calls);
    EXPECT_EQ("ready", client.ReadInstallationState().at("phase"));

    repository.cancel.requested = false;
    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::unknownOutcome, [&] { client.ApplyUpdate(repository.cancel); }));
    EXPECT_EQ(0u, recording.calls);
}

TEST_F(EngineTest, FailedInstallerPreservesItsPhaseAndIsNotReplayed)
{
    repository.PublishApplicationRelease();
    auto installer = std::make_unique<RecordingInstaller>(
        []() -> InstallResult { throw Error(ErrorCode::unknownOutcome, "Synthetic installer outcome"); });
    auto &recording = *installer;
    auto client = CreateClientWithInstaller(std::move(installer));

    ASSERT_TRUE(
        FailsWithUpdateError(ErrorCode::unknownOutcome, [&] { client.ApplyUpdate(repository.cancel); }));

    EXPECT_EQ(1u, recording.calls);
    EXPECT_EQ("applying", client.ReadInstallationState().at("phase"));
    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::unknownOutcome, [&] { client.ApplyUpdate(repository.cancel); }));
    EXPECT_EQ(1u, recording.calls);
}

TEST_F(EngineTest, RebootCompletionRequiresExplicitConfirmation)
{
    repository.PublishApplicationRelease();
    auto installer = std::make_unique<RecordingInstaller>([] { return InstallResult{true, {}}; });
    auto &recording = *installer;
    auto client = CreateClientWithInstaller(std::move(installer));

    const auto result = client.ApplyUpdate(repository.cancel);

    EXPECT_EQ("pendingReboot", result.at("status"));
    EXPECT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ReconcileInstallationOutcome(false); }));
    EXPECT_EQ("complete", client.ReconcileInstallationOutcome(true).at("phase"));
    EXPECT_EQ("upToDate", client.ApplyUpdate(repository.cancel).at("status"));
    EXPECT_EQ(1u, recording.calls);
}

TEST_F(EngineTest, EntryQuotaRejectsAnIncompleteVersionBeforeActivation)
{
    WriteFixtureFile(repository.root / "application/extra.txt", "another file");
    repository.WriteApplicationArchive("synthetic version one");
    repository.PublishApplicationRelease();
    auto client = CreateClientWithInstaller(
        std::make_unique<PortableInstaller>(repository.config.maxExtractedBytes, 1));

    ASSERT_TRUE(
        FailsWithUpdateError(ErrorCode::invalidInput, [&] { client.ApplyUpdate(repository.cancel); }));

    const auto state = client.ReadInstallationState();
    const auto hash = repository.crypto.HashFile(repository.package);

    EXPECT_EQ("applying", state.at("phase"));
    EXPECT_FALSE(state.contains("activePath"));
    EXPECT_FALSE(
        fs::exists(repository.config.installDirectory / "versions" / hash / ".bazupgrade-version.json"));
}

TEST_F(EngineTest, ExtractionAcceptsExactByteAndEntryQuotas)
{
    const std::string contents = "synthetic version one";

    repository.PublishApplicationRelease();
    auto client = CreateClientWithInstaller(std::make_unique<PortableInstaller>(contents.size(), 1));

    const auto result = client.ApplyUpdate(repository.cancel);
    const auto state = client.ReadInstallationState();

    EXPECT_EQ("updated", result.at("status"));
    EXPECT_EQ(contents, ReadFile(fs::u8path(state.at("activePath").get<std::string>())));
}
} // namespace test
