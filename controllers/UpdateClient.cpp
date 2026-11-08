#include "controllers/UpdateClient.hpp"
#include "controllers/NativeInstaller.hpp"
#include "controllers/PortableInstaller.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include "curl/RepositoryTransport.hpp"
#include "interfaces/Version.hpp"
#include "storage/FileLock.hpp"

namespace baz
{
ClientConfig ClientConfig::Load(const fs::path &path)
{
    auto json = ReadJson(path);
    ClientConfig value;
    Require(json.at("schemaVersion") == 1, ErrorCode::unsupported, "Unsupported client configuration");
    value.repository = json.at("repository");
    value.appId = Component(json.at("appId"));
    value.channel = Component(json.at("channel"));
    value.os = Component(json.at("os"));
    value.arch = Component(json.at("arch"));
    value.packageType = Component(json.at("packageType"));
    value.target = json.at("target");
    SafeRelative(value.target);
    auto base = fs::absolute(path).parent_path();
    auto localPath = [&](const std::string &member)
    { return fs::absolute(base / fs::u8path(json.at(member).get<std::string>())).lexically_normal(); };
    value.trustedRoot = localPath("trustedRoot");
    value.stateDirectory = localPath("stateDirectory");
    value.installDirectory = localPath("installDirectory");
    if (value.repository.find("://") == std::string::npos)
        value.repository = fs::absolute(base / fs::u8path(value.repository)).u8string();
    value.allowLoopbackHttp = json.value("allowLoopbackHttp", false);
    value.maxDownloadBytes = json.value("maxDownloadBytes", value.maxDownloadBytes);
    value.maxExtractedBytes = json.value("maxExtractedBytes", value.maxExtractedBytes);
    Require(value.maxDownloadBytes > 0 && value.maxExtractedBytes > 0, ErrorCode::invalidInput,
            "Quotas must be positive");
    return value;
}
UpdateClient::UpdateClient(ClientConfig value)
    : UpdateClient(std::move(value), std::make_unique<OpenSslCrypto>(), nullptr, nullptr)
{
}

UpdateClient::UpdateClient(ClientConfig value, std::unique_ptr<CryptoProvider> verifier,
                           std::unique_ptr<Transport> source, std::unique_ptr<Installer> backend)
    : config(std::move(value)), crypto(std::move(verifier)), transport(std::move(source)),
      installer(std::move(backend))
{
    Require(crypto != nullptr, ErrorCode::invalidInput, "A crypto provider is required");
    Component(config.appId);
    Component(config.channel);
    Component(config.os);
    Component(config.arch);
    Component(config.packageType);
    SafeRelative(config.target);
    Require(installer || config.packageType == "portable" || config.packageType == "electron" ||
                NativeInstaller::Supports(config.packageType),
            ErrorCode::unsupported, "Unsupported installer on this platform");
    SecureDirectory(config.stateDirectory);
    SecureDirectory(config.installDirectory);
    if (!transport)
    {
        if (config.repository.find("://") == std::string::npos)
            transport = std::make_unique<DirectoryTransport>(fs::u8path(config.repository));
        else
            transport = std::make_unique<CurlTransport>(config.repository, config.allowLoopbackHttp);
    }
    if (!installer && config.packageType == "portable")
        installer = std::make_unique<PortableInstaller>(config.maxExtractedBytes);
    else if (!installer && config.packageType != "electron")
        installer = std::make_unique<NativeInstaller>();
    tuf =
        std::make_unique<TufClient>(*transport, *crypto, config.trustedRoot, config.stateDirectory / "trust");
}
void UpdateClient::ValidateState(const Json &state) const
{
    Require(state.at("appId") == config.appId && state.at("channel") == config.channel &&
                state.at("os") == config.os && state.at("arch") == config.arch &&
                state.at("packageType") == config.packageType && state.at("target") == config.target &&
                state.at("installDirectory") == config.installDirectory.generic_u8string() &&
                state.at("bootstrap") == crypto->Hash(CanonicalJson(ReadJson(config.trustedRoot))),
            ErrorCode::trust, "State scope does not match this client configuration");
}
Json UpdateClient::LoadState() const
{
    auto path = config.stateDirectory / "installation.json";
    if (fs::exists(path))
    {
        auto state = ReadJson(path);
        ValidateState(state);
        return state;
    }
    return {{"appId", config.appId},
            {"channel", config.channel},
            {"os", config.os},
            {"arch", config.arch},
            {"packageType", config.packageType},
            {"target", config.target},
            {"installDirectory", config.installDirectory.generic_u8string()},
            {"bootstrap", crypto->Hash(CanonicalJson(ReadJson(config.trustedRoot)))},
            {"sequence", 0},
            {"phase", "idle"}};
}
Target UpdateClient::Resolve(const Cancellation &cancel)
{
    auto target = tuf->Resolve(config.target, cancel);
    const auto &custom = target.custom;
    Require(custom.at("schemaVersion") == 1 && custom.at("appId") == config.appId &&
                custom.at("channel") == config.channel && custom.at("os") == config.os &&
                custom.at("arch") == config.arch && custom.at("packageType") == config.packageType,
            ErrorCode::trust,
            "Signed target does not match selected application, channel, platform or installer");
    Require(custom.at("releaseSequence").is_number_unsigned() ||
                (custom.at("releaseSequence").is_number_integer() &&
                 custom.at("releaseSequence").get<std::int64_t>() > 0),
            ErrorCode::trust, "Invalid release sequence");
    Require(custom.at("releaseSequence").get<std::uint64_t>() > 0 &&
                custom.at("releaseSequence").get<std::uint64_t>() <= 9007199254740991ULL,
            ErrorCode::trust, "Invalid release sequence");
    Require(custom.value("minUpdaterMajor", 2) <= 2, ErrorCode::unsupported,
            "A newer BazUpgrade is required");
    Require(target.length <= config.maxDownloadBytes &&
                target.hashes.at("sha256").get<std::string>().size() == 64,
            ErrorCode::invalidInput, "Artifact exceeds policy or has an invalid hash");
    Unhex(target.hashes.at("sha256"));
    auto trusted = LoadState();
    auto sequence = custom.at("releaseSequence").get<std::uint64_t>();
    auto floor = trusted.value("trustedSequence", trusted.at("sequence").get<std::uint64_t>());
    Require(sequence >= floor, ErrorCode::rollback, "Trusted application release sequence rollback");
    if (sequence == floor && trusted.contains("trustedSha256"))
        Require(trusted.at("trustedSha256") == target.hashes.at("sha256"), ErrorCode::trust,
                "Trusted release sequence reused");
    trusted["trustedSequence"] = sequence;
    trusted["trustedSha256"] = target.hashes.at("sha256");
    WriteJson(config.stateDirectory / "installation.json", trusted);
    return target;
}
Json UpdateClient::Check(const Cancellation &cancel)
{
    FileLock lock(config.stateDirectory / "operation.lock");
    auto state = LoadState();
    auto target = Resolve(cancel);
    state = LoadState();
    auto sequence = target.custom.at("releaseSequence").get<std::uint64_t>();
    auto installed = state.at("sequence").get<std::uint64_t>();
    Require(sequence >= installed, ErrorCode::rollback, "Application release sequence rollback");
    if (sequence == installed && state.contains("sha256"))
        Require(state.value("acceptedSha256", state.value("sha256", "")) == target.hashes.at("sha256"),
                ErrorCode::trust, "Same release sequence identifies different bytes");
    return {{"status", sequence > installed ? "updateAvailable" : "upToDate"},
            {"appId", config.appId},
            {"version", target.custom.at("appVersion")},
            {"releaseSequence", sequence},
            {"sha256", target.hashes.at("sha256")},
            {"length", target.length},
            {"phase", state.at("phase")}};
}
Json UpdateClient::VerifyFile(const fs::path &path, const Cancellation &cancel)
{
    FileLock lock(config.stateDirectory / "operation.lock");
    auto target = Resolve(cancel);
    Require(!fs::is_symlink(fs::symlink_status(path)) && fs::is_regular_file(path) &&
                fs::file_size(path) == target.length,
            ErrorCode::trust, "Downloaded file size mismatch");
    for (auto it = target.hashes.begin(); it != target.hashes.end(); ++it)
        if (it.key() == "sha256" || it.key() == "sha512")
            Require(crypto->HashFile(path, it.key()) == it.value(), ErrorCode::trust,
                    "Downloaded file hash mismatch");
    return {{"status", "verified"},
            {"sha256", target.hashes.at("sha256")},
            {"version", target.custom.at("appVersion")}};
}
Json UpdateClient::Status()
{
    FileLock lock(config.stateDirectory / "operation.lock");
    return LoadState();
}
Json UpdateClient::Apply(const Cancellation &cancel, std::uint64_t waitPid,
                         const std::function<void()> &ready)
{
    Require(waitPid <= 2147483647, ErrorCode::invalidInput, "Process id out of range");
    Require(config.packageType != "electron", ErrorCode::unsupported,
            "Electron targets use native updater integration");
    FileLock stateLock(config.stateDirectory / "operation.lock");
    FileLock installLock(config.installDirectory / "installation.lock");
    auto ownerPath = config.installDirectory / "owner.json";
    Json owner = {{"appId", config.appId}, {"stateDirectory", config.stateDirectory.generic_u8string()}};
    if (fs::exists(ownerPath))
        Require(ReadJson(ownerPath) == owner, ErrorCode::trust,
                "Installation directory belongs to another application");
    else
        WriteJson(ownerPath, owner);
    auto state = LoadState();
    auto phase = state.at("phase").get<std::string>();
    Require(phase == "idle" || phase == "complete", ErrorCode::unknownOutcome,
            "Pending operation requires inspection and explicit reconciliation");
    auto target = Resolve(cancel);
    state = LoadState();
    auto sequence = target.custom.at("releaseSequence").get<std::uint64_t>();
    auto current = state.at("sequence").get<std::uint64_t>();
    Require(sequence >= current, ErrorCode::rollback, "Application release sequence rollback");
    if (sequence == current)
    {
        Require(state.value("acceptedSha256", state.value("sha256", "")) == target.hashes.at("sha256"),
                ErrorCode::trust, "Release sequence reused for different bytes");
        return {{"status", "upToDate"}};
    }
    auto hash = target.hashes.at("sha256").get<std::string>();
    SecureDirectory(config.stateDirectory / "downloads");
    auto extension = config.packageType == "msix"   ? ".msix"
                     : config.packageType == "msi"  ? ".msi"
                     : config.packageType == "nsis" ? ".exe"
                     : config.packageType == "deb"  ? ".deb"
                     : config.packageType == "rpm"  ? ".rpm"
                     : config.packageType == "pkg"  ? ".pkg"
                                                    : ".archive";
    auto artifact = config.stateDirectory / "downloads" / (hash + extension);
    state["pending"] = {
        {"sequence", sequence}, {"version", target.custom.at("appVersion")}, {"sha256", hash}};
    state["phase"] = "downloading";
    WriteJson(config.stateDirectory / "installation.json", state);
    try
    {
        cancel.Check();
        if (!fs::exists(artifact))
        {
            auto partial = artifact;
            partial += ".partial";
            Require(!fs::exists(partial), ErrorCode::io,
                    "Interrupted download exists; inspect and reconcile");
            tuf->Download(target, partial, cancel);
            fs::rename(partial, artifact);
        }
        Require(!fs::is_symlink(fs::symlink_status(artifact)) && fs::file_size(artifact) == target.length &&
                    crypto->HashFile(artifact) == hash,
                ErrorCode::trust, "Cached artifact failed verification");
        state["phase"] = "ready";
        WriteJson(config.stateDirectory / "installation.json", state);
        if (ready)
            ready();
        if (waitPid)
            WaitForProcess(waitPid, cancel);
        cancel.Check();
        state["phase"] = "applying";
        WriteJson(config.stateDirectory / "installation.json", state);
        auto result = installer->Apply(target, artifact, config.installDirectory, cancel);
        if (state.contains("activePath"))
            state["previous"] = {{"activePath", state.at("activePath")},
                                 {"version", state.value("version", "")},
                                 {"sha256", state.value("sha256", "")}};
        state["sequence"] = sequence;
        state["version"] = target.custom.at("appVersion");
        state["sha256"] = hash;
        state["acceptedSha256"] = hash;
        if (!result.activePath.empty())
            state["activePath"] = result.activePath;
        state["phase"] = result.rebootRequired ? "pendingReboot" : "complete";
        state.erase("pending");
        WriteJson(config.stateDirectory / "installation.json", state);
        return {{"status", result.rebootRequired ? "pendingReboot" : "updated"}, {"state", state}};
    }
    catch (...)
    {
        // Preserve the durable phase. Never rerun an installer after an unknown effect.
        throw;
    }
}
Json UpdateClient::Reconcile(bool installed)
{
    FileLock stateLock(config.stateDirectory / "operation.lock");
    FileLock installLock(config.installDirectory / "installation.lock");
    auto state = LoadState();
    Require(state.at("phase") != "idle" && state.at("phase") != "complete", ErrorCode::invalidInput,
            "No interrupted operation to reconcile");
    Require(state.at("phase") != "pendingReboot" || installed, ErrorCode::invalidInput,
            "Observe reboot completion before confirming this installation");
    if (installed)
    {
        if (state.contains("pending"))
        {
            Require(config.packageType != "portable", ErrorCode::unsupported,
                    "Portable activation cannot be asserted; inspect staging and recover as not installed");
            state["sequence"] = state.at("pending").at("sequence");
            state["version"] = state.at("pending").at("version");
            state["sha256"] = state.at("pending").at("sha256");
            state["acceptedSha256"] = state.at("sha256");
        }
    }
    if (!installed && state.contains("pending"))
    {
        auto hash = state.at("pending").at("sha256").get<std::string>();
        Require(hash.size() == 64, ErrorCode::trust, "Invalid pending artifact hash");
        Unhex(hash);
        auto downloads = config.stateDirectory / "downloads";
        if (fs::exists(downloads))
        {
            SecureDirectory(downloads);
            for (const auto &file : fs::directory_iterator(downloads))
                if (file.path().filename().string().rfind(hash + ".", 0) == 0 &&
                    file.path().extension() == ".partial")
                    fs::remove(file.path());
        }
        auto staging = config.installDirectory / "versions" / hash;
        bool active = state.value("sha256", "") == hash ||
                      (state.contains("previous") && state.at("previous").value("sha256", "") == hash);
        if (config.packageType == "portable" && !active && fs::exists(staging))
        {
            SecureDirectory(config.installDirectory / "versions");
            Require(!fs::is_symlink(staging), ErrorCode::io, "Unsafe staging directory");
            Require(staging.parent_path() == config.installDirectory / "versions", ErrorCode::io,
                    "Staging scope mismatch");
            fs::remove_all(staging);
        }
    }
    state.erase("pending");
    state["phase"] = state.at("sequence").get<std::uint64_t>() ? "complete" : "idle";
    WriteJson(config.stateDirectory / "installation.json", state);
    return state;
}
Json UpdateClient::Rollback()
{
    FileLock stateLock(config.stateDirectory / "operation.lock");
    FileLock installLock(config.installDirectory / "installation.lock");
    auto state = LoadState();
    Require(config.packageType == "portable" && state.at("phase") == "complete" && state.contains("previous"),
            ErrorCode::unsupported, "No portable rollback is available");
    auto previous = state.at("previous");
    auto path = fs::u8path(previous.at("activePath").get<std::string>());
    auto relative = path.lexically_relative(config.installDirectory / "versions");
    SafeRelative(relative.generic_u8string());
    SecureDirectory(path.parent_path());
    Require(fs::is_regular_file(path) && !fs::is_symlink(path), ErrorCode::io,
            "Previous installation is unavailable");
    state["previous"] = {{"activePath", state.at("activePath")},
                         {"version", state.at("version")},
                         {"sha256", state.at("sha256")}};
    state["activePath"] = previous.at("activePath");
    state["version"] = previous.at("version");
    state["sha256"] = previous.at("sha256");
    // Keep the sequence high-water mark even when explicitly selecting older files.
    WriteJson(config.stateDirectory / "installation.json", state);
    return state;
}
Json Capabilities()
{
    Json installers = Json::array({"portable"});
    for (const std::string kind : {"msix", "msi", "nsis", "deb", "rpm", "pkg"})
        if (NativeInstaller::Supports(kind))
            installers.push_back(kind);
    return {{"schemaVersion", 1},
            {"updaterMajor", 2},
            {"installers", installers},
            {"crypto", Json::array({"ed25519", "sha256", "sha512"})},
            {"verificationOnly", Json::array({"electron"})},
            {"automaticElevation", false},
            {"portableRollback", "filesOnly"},
            {"nativeRollback", false},
            {"firmware", false}};
}
} // namespace baz
