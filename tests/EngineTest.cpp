#include "curl/RepositoryTransport.hpp"
#include "interfaces/HttpClient.hpp"
#include "tests/TestSupport.hpp"
namespace test
{
class TestHttpClient final : public HTTPClient
{
  public:
    std::function<curlwrapper::Response(const curlwrapper::Request &)> respond;
    curlwrapper::Response Perform(const curlwrapper::Request &request) override
    {
        return respond(request);
    }
    void AddHeader(const std::string &) override
    {
        throw std::logic_error("Unexpected header mutation");
    }
    bool Get(const std::string &, std::string &) override
    {
        throw std::logic_error("Unexpected GET facade");
    }
    bool Post(const std::string &, std::string &, std::string &) override
    {
        throw std::logic_error("Unexpected POST facade");
    }
    bool Download(const std::string &, FILE *) override
    {
        throw std::logic_error("Unexpected download facade");
    }
};

void EngineTests()
{
    Case("HTTP development transport remains loopback only",
         []
         {
             CurlTransport secure("https://example.invalid/repository");
             CurlTransport development("http://127.0.0.1:12345/repository", true);
             CurlTransport ipv6("http://[::1]:12345/repository", true);
             Reject(ErrorCode::invalidInput, [] { CurlTransport transport("http://127.0.0.1/"); });
             Reject(ErrorCode::invalidInput,
                    [] { CurlTransport transport("http://example.invalid/", true); });
             Reject(ErrorCode::invalidInput, [] { CurlTransport transport("ftp://127.0.0.1/", true); });
             Reject(ErrorCode::invalidInput,
                    [] { CurlTransport transport("https://user:synthetic@example.invalid/"); });
             for (const auto &url : {"https://@example.invalid/", "https://example.invalid/?",
                                     "https://example.invalid/#", "http://localhost.example.invalid/",
                                     "http://127.0.0.1.example.invalid/", "https://example.invalid:invalid/"})
                 Reject(ErrorCode::invalidInput, [&] { CurlTransport transport(url, true); });
         });
    Case("repository requests use an injected HTTP interface with updater policy",
         []
         {
             auto client = std::make_unique<TestHttpClient>();
             auto *observed = client.get();
             unsigned calls = 0;
             Cancellation cancel;
             observed->respond = [&](const curlwrapper::Request &request)
             {
                 ++calls;
                 Check(request.url == "https://example.invalid/repository/metadata/a%20b.json",
                       "Path was not encoded");
                 Check(request.method == "GET" && !request.sink, "Metadata request is not a buffered GET");
                 Check(request.options.has_value(), "Updater network options are missing");
                 const auto &options = *request.options;
                 Check(options.httpsOnly && !options.followRedirects && !options.useEnvironmentProxy &&
                           !options.allowUrlCredentials && options.maxBytes == 1024 &&
                           options.timeoutSeconds == 300 && options.lowSpeedLimit == 1 &&
                           options.lowSpeedSeconds == 20,
                       "Updater network policy changed");
                 Check(request.cancelled && !request.cancelled(), "Cancellation was not connected");
                 cancel.requested = true;
                 Check(request.cancelled(), "Cancellation does not observe the current state");
                 return curlwrapper::Response{200, 8, "metadata"};
             };
             CurlTransport transport("https://example.invalid/repository", std::move(client));
             Check(transport.Fetch("metadata/a b.json", 1024, cancel) ==
                       std::optional<std::string>("metadata"),
                   "Metadata was not returned");
             Check(calls == 1, "Request was repeated");
             Reject(ErrorCode::invalidInput, [&] { transport.Fetch("../escape", 1024, cancel); });
             Check(calls == 1, "Unsafe path reached the HTTP client");
         });
    Case("repository HTTP status and client errors retain their application meaning",
         []
         {
             auto client = std::make_unique<TestHttpClient>();
             auto *observed = client.get();
             CurlTransport transport("https://example.invalid/", std::move(client));
             Cancellation cancel;
             observed->respond = [](const curlwrapper::Request &)
             { return curlwrapper::Response{404, 0, {}}; };
             Check(!transport.Fetch("missing.json", 64, cancel), "404 did not mean missing metadata");
             observed->respond = [](const curlwrapper::Request &)
             { return curlwrapper::Response{503, 0, {}}; };
             Reject(ErrorCode::transport, [&] { transport.Fetch("metadata.json", 64, cancel); });
             observed->respond = [](const curlwrapper::Request &) -> curlwrapper::Response
             { throw curlwrapper::Error(curlwrapper::Failure::cancelled, "synthetic cancellation"); };
             Reject(ErrorCode::cancelled, [&] { transport.Fetch("metadata.json", 64, cancel); });
             observed->respond = [](const curlwrapper::Request &) -> curlwrapper::Response
             { throw curlwrapper::Error(curlwrapper::Failure::sizeLimit, "synthetic limit"); };
             Reject(ErrorCode::transport, [&] { transport.Fetch("metadata.json", 64, cancel); });
         });
    Case("artifact streaming uses the same injected client and rejects existing destinations",
         []
         {
             Fixture fixture;
             auto client = std::make_unique<TestHttpClient>();
             unsigned calls = 0;
             client->respond = [&](const curlwrapper::Request &request)
             {
                 ++calls;
                 Check(request.sink && request.options && request.options->maxBytes == 5,
                       "Bounded stream is missing");
                 const std::string bytes("a\0bcd", 5);
                 Check(request.sink(bytes.data(), bytes.size()), "Artifact sink rejected valid bytes");
                 return curlwrapper::Response{200, 5, {}};
             };
             CurlTransport transport("https://example.invalid/", std::move(client));
             auto destination = fixture.root / "download.bin";
             transport.Download("artifact.bin", destination, 5, fixture.cancel);
             Check(ReadFile(destination) == std::string("a\0bcd", 5), "Binary artifact was damaged");
             Reject(ErrorCode::io,
                    [&] { transport.Download("artifact.bin", destination, 5, fixture.cancel); });
             Check(calls == 1, "Existing destination caused another transfer");
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
