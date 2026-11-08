#include "curl/RepositoryTransport.hpp"
#include "tests/TestSupport.hpp"
namespace test
{
void EngineTests()
{
    Case("HTTP development transport remains loopback only",
         []
         {
             CurlTransport secure("https://example.invalid/repository");
             CurlTransport development("http://127.0.0.1:12345/repository", true);
             Reject(ErrorCode::invalidInput, [] { CurlTransport transport("http://127.0.0.1/"); });
             Reject(ErrorCode::invalidInput,
                    [] { CurlTransport transport("http://example.invalid/", true); });
             Reject(ErrorCode::invalidInput, [] { CurlTransport transport("ftp://127.0.0.1/", true); });
             Reject(ErrorCode::invalidInput,
                    [] { CurlTransport transport("https://user:synthetic@example.invalid/"); });
         });
    Case("portable update activates an immutable version",
         []
         {
             Fixture fixture;
             fixture.Publish();
             auto result = UpdateClient(fixture.config).Apply(fixture.cancel);
             Check(result.at("status") == "updated", "Update failed");
             auto state = UpdateClient(fixture.config).Status();
             Check(ReadFile(fs::u8path(state.at("activePath").get<std::string>())) == "synthetic version one",
                   "Wrong active file");
             Check(UpdateClient(fixture.config).Apply(fixture.cancel).at("status") == "upToDate",
                   "Completed update replayed");
         });
    Case("release selection uses sequence rather than lexical version",
         []
         {
             Fixture fixture;
             auto descriptor = fixture.Custom(1);
             descriptor["appVersion"] = "1.9.0";
             RepositoryPublisher(fixture.repo, fixture.keys)
                 .Publish(fixture.package, fixture.config.target, descriptor);
             UpdateClient(fixture.config).Apply(fixture.cancel);
             Bytes(fixture.root / "application/app.txt", "synthetic version two");
             fs::remove(fixture.package);
             PackDirectory(fixture.root / "application", fixture.package);
             descriptor = fixture.Custom(2);
             descriptor["appVersion"] = "1.10.0";
             RepositoryPublisher(fixture.repo, fixture.keys)
                 .Publish(fixture.package, fixture.config.target, descriptor);
             Check(UpdateClient(fixture.config).Check(fixture.cancel).at("version") == "1.10.0",
                   "Double-digit version was skipped");
         });
    Case("a selected target cannot cross application scope",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.config.appId = "another-app";
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
         });
    Case("state cannot be reused with another channel",
         []
         {
             Fixture fixture;
             fixture.Publish();
             UpdateClient(fixture.config).Check(fixture.cancel);
             fixture.config.channel = "beta";
             Reject(ErrorCode::trust, [&] { UpdateClient(fixture.config).Status(); });
         });
    Case("operation lock does not block unrelated applications",
         []
         {
             Fixture first, second;
             first.Publish();
             second.Publish();
             FileLock active(first.config.stateDirectory / "operation.lock");
             Reject(ErrorCode::busy, [&] { UpdateClient(first.config).Check(first.cancel); });
             Check(UpdateClient(second.config).Check(second.cancel).at("status") == "updateAvailable",
                   "Unrelated operation was blocked");
         });
    Case("pre-cancelled update leaves no installer pending",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.cancel.requested = true;
             Reject(ErrorCode::cancelled, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
             Check(UpdateClient(fixture.config).Status().at("phase") == "idle",
                   "Cancelled check changed installation phase");
         });
    Case("unknown operation is never automatically replayed",
         []
         {
             Fixture fixture;
             fixture.Publish();
             UpdateClient client(fixture.config);
             auto state = client.Status();
             state["phase"] = "applying";
             state["pending"] = {
                 {"sequence", 1}, {"version", "1.1.0"}, {"sha256", fixture.crypto.HashFile(fixture.package)}};
             WriteJson(fixture.config.stateDirectory / "installation.json", state);
             Reject(ErrorCode::unknownOutcome, [&] { client.Apply(fixture.cancel); });
             Check(client.Reconcile(false).at("phase") == "idle", "Explicit reconciliation failed");
         });
    Case("archive traversal is rejected",
         []
         {
             Fixture fixture;
             fs::remove(fixture.package);
             MaliciousZip(fixture.package, "../outside.txt");
             fixture.Publish();
             Reject(ErrorCode::invalidInput, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
             Check(!fs::exists(fixture.config.installDirectory / "outside.txt"),
                   "Archive escaped its version directory");
         });
    Case("archive symlinks are rejected",
         []
         {
             Fixture fixture;
             fs::remove(fixture.package);
             MaliciousZip(fixture.package, "app.txt", true);
             fixture.Publish();
             Reject(ErrorCode::invalidInput, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
         });
    Case("extraction quota is enforced independently of compressed size",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.config.maxExtractedBytes = 1;
             Reject(ErrorCode::invalidInput, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
         });
    Case("download quota is enforced before fetching an artifact",
         []
         {
             Fixture fixture;
             fixture.Publish();
             fixture.config.maxDownloadBytes = 1;
             Reject(ErrorCode::invalidInput, [&] { UpdateClient(fixture.config).Apply(fixture.cancel); });
         });
    Case("an external updater can verify a downloaded artifact",
         []
         {
             Fixture fixture;
             fixture.Publish();
             Check(UpdateClient(fixture.config).VerifyFile(fixture.package, fixture.cancel).at("status") ==
                       "verified",
                   "Verification integration failed");
             Bytes(fixture.root / "wrong.zip", "wrong");
             Reject(ErrorCode::trust, [&]
                    { UpdateClient(fixture.config).VerifyFile(fixture.root / "wrong.zip", fixture.cancel); });
         });
    Case("publisher rejects sequence reuse",
         []
         {
             Fixture fixture;
             fixture.Publish();
             Reject(ErrorCode::rollback, [&] { fixture.Publish(); });
         });
    Case("capabilities distinguish native installation and verification",
         []
         {
             auto result = Capabilities();
             Check(result.at("automaticElevation") == false && result.at("firmware") == false,
                   "Unsupported capabilities advertised");
             Check(result.at("verificationOnly").at(0) == "electron",
                   "Verification adapter is not discoverable");
         });
}
} // namespace test
