#include "storage/MetadataDirectory.hpp"
#include "tests/TestSupport.hpp"
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif
namespace test
{
void TrustTests()
{
    Case("canonical JSON sorts keys and preserves UTF-8",
         []
         {
             Check(CanonicalJson(ParseJson(R"({"z":1,"a":"line\nnext","b":true})")) ==
                       std::string("{\"a\":\"line\nnext\",\"b\":true,\"z\":1}"),
                   "Wrong canonical serialization");
             Reject(ErrorCode::trust, [] { CanonicalJson(Json(1.5)); });
             Reject(ErrorCode::trust, [] { ParseJson(R"({"a":1,"a":2})"); });
         });
    Case("Ed25519 known vector rejects changed messages",
         []
         {
             OpenSslCrypto crypto;
             Json key = {{"keytype", "ed25519"},
                         {"scheme", "ed25519"},
                         {"keyval",
                          {{"public", "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a"}}}};
             std::string signature = "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821"
                                     "590a33bacc61e39701cf9b46bd25bf5f0595bbe24655141438e7a100b";
             Check(crypto.Verify(key, "", signature), "RFC 8032 vector rejected");
             Check(!crypto.Verify(key, "changed", signature), "Changed message accepted");
         });
    Case("signed publication resolves and downloads",
         []
         {
             Fixture fixture;
             fixture.Publish();
             DirectoryTransport transport(fixture.repo);
             TufClient client(transport, fixture.crypto, fixture.bootstrap, fixture.root / "trust");
             auto target = client.Resolve(fixture.config.target, fixture.cancel);
             Check(target.custom.at("releaseSequence") == 1, "Wrong release");
             client.Download(target, fixture.root / "download.zip", fixture.cancel);
             Check(fixture.crypto.HashFile(fixture.root / "download.zip") ==
                       fixture.crypto.HashFile(fixture.package),
                   "Downloaded bytes changed");
         });
    Case("threshold signatures count unique authorized keys",
         []
         {
             Fixture fixture;
             auto envelope = ReadJson(fixture.bootstrap), body = envelope.at("signed");
             envelope["signatures"] =
                 Json::array({envelope.at("signatures").at(0), envelope.at("signatures").at(0)});
             Reject(ErrorCode::trust,
                    [&] {
                        VerifyThreshold(fixture.crypto, envelope, body.at("keys"),
                                        body.at("roles").at("root"));
                    });
         });
    Case("changed signed body fails verification",
         []
         {
             Fixture fixture;
             auto envelope = ReadJson(fixture.bootstrap), authority = envelope.at("signed");
             envelope["signed"]["expires"] = "2099-01-01T00:00:00Z";
             Reject(ErrorCode::trust,
                    [&] {
                        VerifyThreshold(fixture.crypto, envelope, authority.at("keys"),
                                        authority.at("roles").at("root"));
                    });
         });
    Case("expired timestamp fails closed",
         []
         {
             Fixture fixture;
             fixture.Publish();
             auto timestamp = ReadJson(fixture.repo / "metadata/timestamp.json").at("signed");
             timestamp["expires"] = "2000-01-01T00:00:00Z";
             fixture.Resign("timestamp.json", timestamp, "timestamp");
             UpdateClient client(fixture.config);
             Reject(ErrorCode::expired, [&] { client.Check(fixture.cancel); });
         });
    Case("metadata version rollback survives a new client",
         []
         {
             Fixture fixture;
             fixture.Publish();
             auto old = ReadJson(fixture.repo / "metadata/timestamp.json");
             UpdateClient(fixture.config).Check(fixture.cancel);
             fixture.Publish(2);
             UpdateClient(fixture.config).Check(fixture.cancel);
             WriteJson(fixture.repo / "metadata/timestamp.json", old);
             Reject(ErrorCode::rollback, [&] { UpdateClient(fixture.config).Check(fixture.cancel); });
         });
    Case("metadata cannot change at the same version",
         []
         {
             Fixture fixture;
             fixture.Publish();
             UpdateClient(fixture.config).Check(fixture.cancel);
             auto timestamp = ReadJson(fixture.repo / "metadata/timestamp.json").at("signed");
             timestamp["expires"] = "2099-01-01T00:00:00Z";
             fixture.Resign("timestamp.json", timestamp, "timestamp");
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Check(fixture.cancel); });
         });
    Case("root rotation works from the original bootstrap",
         []
         {
             Fixture fixture;
             fixture.Publish();
             UpdateClient(fixture.config).Check(fixture.cancel);
             RepositoryPublisher(fixture.repo, fixture.keys).RotateRoot();
             Check(UpdateClient(fixture.config).Check(fixture.cancel).at("status") == "updateAvailable",
                   "Rotation lost target");
             Check(ReadJson(fixture.config.stateDirectory / "trust/root.json").at("signed").at("version") ==
                       2,
                   "New root was not persisted");
         });
    Case("corrupted artifact is rejected before extraction",
         []
         {
             Fixture fixture;
             fixture.Publish();
             auto target = ReadJson(fixture.repo / "metadata/targets.json")
                               .at("signed")
                               .at("targets")
                               .at(fixture.config.target);
             auto path = fixture.repo / "targets" /
                         (target.at("hashes").at("sha256").get<std::string>() + "." + fixture.config.target);
             auto bytes = ReadFile(path);
             bytes[0] ^= 1;
             Bytes(path, bytes);
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
             Check(!fs::exists(fixture.config.installDirectory / "versions"),
                   "Unverified archive was extracted");
         });
    Case("missing target is a controlled error",
         []
         {
             Fixture fixture;
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Check(fixture.cancel); });
         });
    Case("delegated role cache cannot alias the trusted root",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.Delegate("ROOT", "ex[a-z]mple.?ip", false, true);
             auto original = ReadJson(fixture.bootstrap);
             UpdateClient(fixture.config).Check(fixture.cancel);
             auto trust = fixture.config.stateDirectory / "trust";
             Check(ReadJson(trust / "root.json") == original, "Delegation replaced trust root");
             Check(fs::exists(trust / "delegated" / (fixture.crypto.Hash("ROOT") + ".json")),
                   "Delegated cache is not isolated");
             UpdateClient(fixture.config).Check(fixture.cancel);
         });
    Case("terminating bracket delegation stops later roles",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.Delegate("first", "ex[!0-9]mple.*", true, false);
             fixture.Delegate("later", "*", false, false);
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Check(fixture.cancel); });
             Check(!fs::exists(fixture.config.stateDirectory / "trust/delegated" /
                               (fixture.crypto.Hash("later") + ".json")),
                   "Terminated traversal visited a later role");
         });
    Case("publisher keeps keys outside the web root",
         []
         {
             Fixture fixture;
             Reject(ErrorCode::invalidInput,
                    [&] { RepositoryPublisher invalid(fixture.repo, fixture.repo / "keys"); });
         });
    Case("publisher rejects replayed metadata and restores protected publication",
         []
         {
             Fixture fixture;
             fixture.Publish();
             std::vector<Json> old;
             for (const std::string role : {"targets", "snapshot", "timestamp"})
                 old.push_back(ReadJson(fixture.repo / "metadata" / (role + ".json")));
             fixture.Publish(2);
             auto checkpoint = ReadJson(fixture.keys / "publication.json");
             unsigned index = 0;
             for (const std::string role : {"targets", "snapshot", "timestamp"})
                 WriteJson(fixture.repo / "metadata" / (role + ".json"), old.at(index++));
             for (unsigned attempt = 0; attempt < 4; ++attempt)
                 Reject(ErrorCode::rollback,
                        [&] { RepositoryPublisher(fixture.repo, fixture.keys).Refresh(); });
             Check(ReadJson(fixture.keys / "publication.json") == checkpoint,
                   "Rejected replay changed signing state");
             RepositoryPublisher(fixture.repo, fixture.keys).RestoreMetadata();
             Check(UpdateClient(fixture.config).Check(fixture.cancel).at("releaseSequence") == 2,
                   "Restoration lost the latest release");
         });
#ifdef _WIN32
    Case("publisher rejects Windows case aliases inside its served root",
         []
         {
             Fixture fixture;
             auto alias = fixture.repo.parent_path() / "REPOSITORY" / "private-nested";
             Reject(ErrorCode::invalidInput, [&] { RepositoryPublisher invalid(fixture.repo, alias); });
             Check(!fs::exists(alias), "Rejected key directory was created");
         });
#endif
    Case("publisher refuses independent snapshot and timestamp replays",
         []
         {
             for (const std::string role : {"snapshot", "timestamp"})
             {
                 Fixture fixture;
                 fixture.Publish();
                 auto path = fixture.repo / "metadata" / (role + ".json");
                 auto previous = ReadJson(path);
                 fixture.Publish(2);
                 auto protectedState = ReadJson(fixture.keys / "publication.json");
                 WriteJson(path, previous);
                 Reject(ErrorCode::rollback,
                        [&] { RepositoryPublisher(fixture.repo, fixture.keys).Refresh(); });
                 Check(ReadJson(fixture.keys / "publication.json") == protectedState,
                       "Refused replay changed protected state");
             }
         });
    Case("interrupted publication requires explicit exact restoration",
         []
         {
             Fixture fixture;
             fixture.Publish();
             auto oldCheckpoint = ReadJson(fixture.keys / "publication.json");
             auto next =
                 oldCheckpoint.at("envelopes").at("snapshot").at("signed").at("version").get<unsigned>() + 1;
             auto obstruction = fixture.repo / "metadata" / (std::to_string(next) + ".snapshot.json");
             fs::create_directory(obstruction);
             Reject(ErrorCode::io, [&] { RepositoryPublisher(fixture.repo, fixture.keys).Refresh(); });
             auto checkpoint = ReadJson(fixture.keys / "publication.json");
             Check(checkpoint != oldCheckpoint, "Interrupted publication lost protected intent");
             Reject(ErrorCode::rollback, [&] { RepositoryPublisher(fixture.repo, fixture.keys).Refresh(); });
             fs::remove(obstruction);
             RepositoryPublisher(fixture.repo, fixture.keys).RestoreMetadata();
             for (const std::string role : {"targets", "snapshot", "timestamp"})
                 Check(ReadJson(fixture.repo / "metadata" / (role + ".json")) ==
                           checkpoint.at("envelopes").at(role),
                       "Restore changed signed bytes");
             RepositoryPublisher(fixture.repo, fixture.keys).Refresh();
             Check(ReadJson(fixture.keys / "publication.json")
                           .at("envelopes")
                           .at("timestamp")
                           .at("signed")
                           .at("version") ==
                       checkpoint.at("envelopes").at("timestamp").at("signed").at("version").get<unsigned>() +
                           1,
                   "Refresh did not advance restored state");
         });
#ifdef _WIN32
    Case("metadata directory cannot be replaced during a transaction",
         []
         {
             Fixture fixture;
             MetadataDirectory directory(fixture.repo);
             std::error_code error;
             fs::rename(fixture.repo / "metadata", fixture.repo / "detached-metadata", error);
             Check(static_cast<bool>(error), "Directory replacement bypassed the transaction anchor");
             directory.Write("anchor.json", {{"value", "synthetic"}});
             Check(fs::exists(fixture.repo / "metadata/anchor.json"), "Anchored write failed");
         });
    Case("metadata handles reject Windows junction redirection",
         []
         {
             Fixture fixture;
             auto outside = fixture.root / "outside";
             fs::create_directory(outside);
             MetadataDirectory directory(fixture.repo);
             for (const auto &entry : fs::directory_iterator(fixture.repo / "metadata"))
                 fs::remove(entry.path());
             auto target = L"\\??\\" + outside.wstring();
             auto display = outside.wstring();
             struct Junction
             {
                 DWORD tag;
                 USHORT length, reserved, substituteOffset, substituteLength, printOffset, printLength;
             };
             auto pathBytes = (target.size() + display.size() + 2) * sizeof(wchar_t);
             std::vector<BYTE> buffer(sizeof(Junction) + pathBytes);
             auto junction = reinterpret_cast<Junction *>(buffer.data());
             junction->tag = IO_REPARSE_TAG_MOUNT_POINT;
             junction->length = static_cast<USHORT>(8 + pathBytes);
             junction->substituteLength = static_cast<USHORT>(target.size() * sizeof(wchar_t));
             junction->printOffset = static_cast<USHORT>((target.size() + 1) * sizeof(wchar_t));
             junction->printLength = static_cast<USHORT>(display.size() * sizeof(wchar_t));
             auto paths = reinterpret_cast<wchar_t *>(buffer.data() + sizeof(Junction));
             std::copy(target.begin(), target.end(), paths);
             std::copy(display.begin(), display.end(), paths + target.size() + 1);
             auto handle = CreateFileW((fixture.repo / "metadata").c_str(), GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                                       FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
             Check(handle != INVALID_HANDLE_VALUE, "Cannot create isolated junction fixture");
             DWORD returned = 0;
             auto created =
                 DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, buffer.data(),
                                 static_cast<DWORD>(buffer.size()), nullptr, 0, &returned, nullptr);
             CloseHandle(handle);
             Check(created != FALSE, "Cannot set isolated junction fixture");
             try
             {
                 directory.Write("anchor.json", {{"value", "synthetic"}});
             }
             catch (const Error &error)
             {
                 Check(error.code == ErrorCode::io, "Unexpected junction rejection");
             }
             Check(!fs::exists(outside / "anchor.json"), "Metadata write followed a changed junction");
         });
#else
    Case("metadata writes remain anchored across directory replacement",
         []
         {
             Fixture fixture;
             auto outside = fixture.root / "outside";
             fs::create_directory(outside);
             MetadataDirectory directory(fixture.repo);
             fs::rename(fixture.repo / "metadata", fixture.repo / "detached-metadata");
             fs::create_directory_symlink(outside, fixture.repo / "metadata");
             directory.Write("anchor.json", {{"value", "synthetic"}});
             Check(!fs::exists(outside / "anchor.json"), "Write followed a replaced directory");
             Check(fs::exists(fixture.repo / "detached-metadata/anchor.json"),
                   "Write lost its original directory anchor");
             Reject(ErrorCode::io, [&] { MetadataDirectory redirected(fixture.repo); });
         });
#endif
    Case("calendar dates and path variants are validated",
         []
         {
             Reject(ErrorCode::trust, [] { ParseUtc("2026-02-30T00:00:00Z"); });
             for (const std::string path : {"../escape", "/absolute", "C:/drive", "a\\\\b", "a%2fb",
                                            "CON.txt", "a//b", "a/../b", "a."})
                 Reject(ErrorCode::invalidInput, [&] { SafeRelative(path); });
         });
}
} // namespace test
