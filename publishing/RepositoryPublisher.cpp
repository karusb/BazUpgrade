#include "publishing/RepositoryPublisher.hpp"
#include "controllers/PortableInstaller.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include "storage/FileLock.hpp"
#include "storage/MetadataDirectory.hpp"
#include "tuf/Metadata.hpp"
#include <chrono>
#include <iomanip>
#include <openssl/evp.h>
#include <sstream>

namespace baz
{
namespace
{
std::string Expires(int days)
{
    auto value = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &value);
#else
    gmtime_r(&value, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}
Json Metadata(const std::string &type, std::uint64_t version, int days)
{
    return {{"_type", type}, {"spec_version", "1.0.37"}, {"version", version}, {"expires", Expires(days)}};
}
Json Description(const Json &envelope)
{
    OpenSslCrypto crypto;
    auto bytes = envelope.dump(2) + '\n';
    return {{"version", envelope.at("signed").at("version")},
            {"length", bytes.size()},
            {"hashes", {{"sha256", crypto.Hash(bytes)}}}};
}
std::string BaseUrl(const std::string &value)
{
    Require(value.rfind("https://", 0) == 0 && value.find_first_of("@?#\r\n") == std::string::npos,
            ErrorCode::invalidInput, "Exported feeds require a public HTTPS base URL");
    return value.back() == '/' ? value : value + '/';
}
std::string Xml(const std::string &value)
{
    std::string output;
    for (char c : value)
    {
        if (c == '&')
            output += "&amp;";
        else if (c == '<')
            output += "&lt;";
        else if (c == '>')
            output += "&gt;";
        else if (c == '"')
            output += "&quot;";
        else if (c == '\'')
            output += "&apos;";
        else
            output += c;
    }
    return output;
}
} // namespace
RepositoryPublisher::RepositoryPublisher(fs::path root, fs::path privateDirectory)
    : repository(fs::absolute(std::move(root)).lexically_normal()),
      keys(fs::absolute(std::move(privateDirectory)).lexically_normal())
{
    SecureDirectory(repository);
    // File identity handles Windows case aliases and short (8.3) names.
    for (auto ancestor = keys; !ancestor.empty();)
    {
        Require(!fs::exists(ancestor) || !fs::equivalent(repository, ancestor), ErrorCode::invalidInput,
                "Signing keys must be outside the served repository");
        auto parent = ancestor.parent_path();
        if (parent == ancestor)
            break;
        ancestor = std::move(parent);
    }
    SecureDirectory(keys);
}
std::vector<fs::path> RepositoryPublisher::RoleKeys(const Json &ring, const std::string &role) const
{
    std::vector<fs::path> output;
    for (const auto &name : ring.at("privateKeys").at(role))
        output.push_back(keys / SafeRelative(name));
    return output;
}
Json RepositoryPublisher::Initialize()
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    Require(!fs::exists(repository / "metadata/root.json") && !fs::exists(keys / "keyring.json"),
            ErrorCode::invalidInput, "Repository or signing keyring already initialized");
    Json ring = {{"privateKeys", Json::object()}};
    Json root = Metadata("root", 1, 365);
    root["consistent_snapshot"] = true;
    root["keys"] = Json::object();
    root["roles"] = Json::object();
    OpenSslCrypto crypto;
    for (const std::string role : {"root", "targets", "snapshot", "timestamp"})
    {
        Json ids = Json::array(), files = Json::array();
        int count = role == "root" ? 3 : 1;
        for (int i = 0; i < count; ++i)
        {
            auto name = role + "-1-" + std::to_string(i) + ".pem";
            auto publicKey = GenerateEd25519Key(keys / name);
            auto id = crypto.Hash(CanonicalJson(publicKey));
            root["keys"][id] = publicKey;
            ids.push_back(id);
            files.push_back(name);
        }
        root["roles"][role] = {{"keyids", ids}, {"threshold", role == "root" ? 2 : 1}};
        ring["privateKeys"][role] = files;
    }
    WriteJson(keys / "keyring.json", ring);
    auto envelope = Sign(root, RoleKeys(ring, "root"));
    WriteJson(keys / "trusted-root.json", envelope);
    MetadataDirectory directory(repository);
    directory.Write("1.root.json", envelope);
    directory.Write("root.json", envelope);
    auto targets = Metadata("targets", 1, 30);
    targets["targets"] = Json::object();
    CommitTargets(targets);
    return {
        {"status", "initialized"}, {"rootThreshold", 2}, {"rootKeys", 3}, {"root", "metadata/1.root.json"}};
}
Json RepositoryPublisher::ReadSigned(const std::string &role)
{
    Require(fs::exists(keys / "publication.json"), ErrorCode::trust,
            "Protected publication checkpoint is missing; restore a trusted signing-state backup");
    auto envelope = MetadataDirectory(repository).Read(role + ".json");
    auto root = ReadJson(keys / "trusted-root.json").at("signed");
    OpenSslCrypto crypto;
    VerifyThreshold(crypto, envelope, root.at("keys"), root.at("roles").at(role));
    Require(envelope.at("signed").at("_type") == role, ErrorCode::trust, "Unexpected published role");
    auto checkpoint = ReadJson(keys / "publication.json");
    Require(checkpoint.at("schemaVersion") == 1 &&
                CanonicalJson(checkpoint.at("envelopes").at(role)) == CanonicalJson(envelope),
            ErrorCode::rollback,
            "Served metadata differs from protected signing state; inspect and run restore-metadata");
    return envelope.at("signed");
}
void RepositoryPublisher::WriteOnlineMetadata(const Json &checkpoint)
{
    const auto &envelopes = checkpoint.at("envelopes");
    MetadataDirectory directory(repository);
    for (const std::string role : {"targets", "snapshot"})
    {
        const auto &envelope = envelopes.at(role);
        auto version = envelope.at("signed").at("version").get<std::uint64_t>();
        directory.Write(std::to_string(version) + "." + role + ".json", envelope);
        directory.Write(role + ".json", envelope);
    }
    directory.Write("timestamp.json", envelopes.at("timestamp"));
}
Json RepositoryPublisher::CommitTargets(Json body)
{
    auto ring = ReadJson(keys / "keyring.json");
    bool initialized = fs::exists(keys / "publication.json");
    auto snapshotVersion = initialized ? ReadSigned("snapshot").at("version").get<std::uint64_t>() + 1 : 1;
    auto timestampVersion = initialized ? ReadSigned("timestamp").at("version").get<std::uint64_t>() + 1 : 1;
    body["expires"] = Expires(30);
    auto targets = Sign(body, RoleKeys(ring, "targets"));
    auto version = body.at("version").get<std::uint64_t>();
    auto snapshot = Metadata("snapshot", snapshotVersion, 7);
    snapshot["meta"] = {{"targets.json", Description(targets)}};
    auto signedSnapshot = Sign(snapshot, RoleKeys(ring, "snapshot"));
    auto timestamp = Metadata("timestamp", timestampVersion, 1);
    timestamp["meta"] = {{"snapshot.json", Description(signedSnapshot)}};
    Json checkpoint = {{"schemaVersion", 1},
                       {"envelopes",
                        {{"targets", targets},
                         {"snapshot", signedSnapshot},
                         {"timestamp", Sign(timestamp, RoleKeys(ring, "timestamp"))}}}};
    Require(checkpoint.dump(2).size() + 1 <= 4 * 1024 * 1024, ErrorCode::invalidInput,
            "Combined publisher metadata exceeds 4 MiB checkpoint quota");
    // Persist intent outside the served tree before making it visible. Failed
    // public writes require explicit restoration, never re-signing stale input.
    WriteJson(keys / "publication.json", checkpoint);
    WriteOnlineMetadata(checkpoint);
    return {{"status", "published"}, {"targetsVersion", version}, {"timestampVersion", timestampVersion}};
}
Json RepositoryPublisher::Publish(const fs::path &artifact, const std::string &targetPath, const Json &custom)
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto target = SafeRelative(targetPath);
    for (const std::string field : {"appId", "channel", "os", "arch", "packageType"})
        Component(custom.at(field));
    Require(custom.at("schemaVersion") == 1 && custom.at("releaseSequence").get<std::uint64_t>() > 0 &&
                custom.at("releaseSequence").get<std::uint64_t>() <= 9007199254740991ULL,
            ErrorCode::invalidInput, "Invalid release descriptor");
    Require(custom.at("appVersion").is_string() && !custom.at("appVersion").get<std::string>().empty(),
            ErrorCode::invalidInput, "Application version is required");
    if (custom.at("packageType") == "portable")
        SafeRelative(custom.at("entrypoint"));
    auto targets = ReadSigned("targets");
    if (targets.at("targets").contains(targetPath))
        Require(
            custom.at("releaseSequence").get<std::uint64_t>() >
                targets.at("targets").at(targetPath).at("custom").at("releaseSequence").get<std::uint64_t>(),
            ErrorCode::rollback, "Release sequence must strictly increase");
    OpenSslCrypto crypto;
    auto hash = crypto.HashFile(artifact);
    auto destination =
        repository / "targets" / target.parent_path() / (hash + "." + target.filename().string());
    SecureDirectory(destination.parent_path());
    if (!fs::exists(destination))
        fs::copy_file(artifact, destination);
#ifndef _WIN32
    fs::permissions(destination, fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read |
                                     fs::perms::others_read);
#endif
    Require(!fs::is_symlink(fs::symlink_status(destination)) && crypto.HashFile(destination) == hash,
            ErrorCode::trust, "Published artifact changed during transfer");
    targets["targets"][targetPath] = {
        {"length", fs::file_size(destination)},
        {"hashes", {{"sha256", hash}, {"sha512", crypto.HashFile(destination, "sha512")}}},
        {"custom", custom}};
    targets["version"] = targets.at("version").get<std::uint64_t>() + 1;
    return CommitTargets(targets);
}
Json RepositoryPublisher::Refresh()
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto targets = ReadSigned("targets");
    targets["version"] = targets.at("version").get<std::uint64_t>() + 1;
    return CommitTargets(targets);
}
Json RepositoryPublisher::RestoreMetadata()
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto checkpoint = ReadJson(keys / "publication.json");
    Require(checkpoint.at("schemaVersion") == 1, ErrorCode::trust, "Invalid publication checkpoint");
    auto root = ReadJson(keys / "trusted-root.json").at("signed");
    OpenSslCrypto crypto;
    const auto &envelopes = checkpoint.at("envelopes");
    for (const std::string role : {"targets", "snapshot", "timestamp"})
    {
        Require(envelopes.at(role).at("signed").at("_type") == role, ErrorCode::trust,
                "Unexpected checkpoint role");
        VerifyThreshold(crypto, envelopes.at(role), root.at("keys"), root.at("roles").at(role));
    }
    Require(envelopes.at("snapshot").at("signed").at("meta").at("targets.json") ==
                    Description(envelopes.at("targets")) &&
                envelopes.at("timestamp").at("signed").at("meta").at("snapshot.json") ==
                    Description(envelopes.at("snapshot")),
            ErrorCode::trust, "Inconsistent publication checkpoint");
    WriteOnlineMetadata(checkpoint);
    return {{"status", "restored"}, {"targetsVersion", envelopes.at("targets").at("signed").at("version")}};
}
Json RepositoryPublisher::RotateRoot()
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto ring = ReadJson(keys / "keyring.json");
    auto old = ReadJson(keys / "trusted-root.json");
    auto root = old.at("signed");
    auto version = root.at("version").get<std::uint64_t>() + 1;
    root["version"] = version;
    root["expires"] = Expires(365);
    auto oldKeys = RoleKeys(ring, "root");
    for (const auto &id : root.at("roles").at("root").at("keyids"))
        root["keys"].erase(id.get<std::string>());
    Json ids = Json::array(), filenames = Json::array();
    OpenSslCrypto crypto;
    for (int i = 0; i < 3; ++i)
    {
        auto name = "root-" + std::to_string(version) + "-" + std::to_string(i) + ".pem";
        auto key = GenerateEd25519Key(keys / name);
        auto id = crypto.Hash(CanonicalJson(key));
        root["keys"][id] = key;
        ids.push_back(id);
        filenames.push_back(name);
    }
    root["roles"]["root"] = {{"keyids", ids}, {"threshold", 2}};
    ring["privateKeys"]["root"] = filenames;
    auto newKeys = RoleKeys(ring, "root");
    oldKeys.insert(oldKeys.end(), newKeys.begin(), newKeys.end());
    auto envelope = Sign(root, oldKeys);
    MetadataDirectory directory(repository);
    directory.Write(std::to_string(version) + ".root.json", envelope);
    directory.Write("root.json", envelope);
    WriteJson(keys / "trusted-root.json", envelope);
    WriteJson(keys / "keyring.json", ring);
    return {{"status", "rotated"}, {"rootVersion", version}};
}
Json RepositoryPublisher::ExportElectron(const std::string &targetPath, const std::string &baseUrl,
                                         const fs::path &output)
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto file = ReadSigned("targets").at("targets").at(targetPath);
    auto target = SafeRelative(targetPath);
    auto name = file.at("hashes").at("sha256").get<std::string>() + "." + target.filename().string();
    auto hash = Unhex(file.at("hashes").at("sha512"));
    std::string base64(4 * ((hash.size() + 2) / 3), '\0');
    EVP_EncodeBlock(reinterpret_cast<unsigned char *>(base64.data()),
                    reinterpret_cast<const unsigned char *>(hash.data()), static_cast<int>(hash.size()));
    auto url = BaseUrl(baseUrl) + "targets/" + (target.parent_path() / name).generic_string();
    Json feed = {{"version", file.at("custom").at("appVersion")},
                 {"files", Json::array({{{"url", url}, {"sha512", base64}, {"size", file.at("length")}}})},
                 {"path", url},
                 {"sha512", base64},
                 {"releaseDate", UtcNow()}};
    WriteJson(output, feed);
    return {{"status", "exported"},
            {"format", "electron-yaml-json"},
            {"tufEnforcedByNativeFeed", false},
            {"manifestSignature", "notGenerated"}};
}
Json RepositoryPublisher::ExportAppInstaller(const std::string &targetPath, const std::string &baseUrl,
                                             const fs::path &output)
{
    FileLock lock(keys / "publish.lock");
    FileLock repositoryLock(repository / ".publish.lock");
    auto file = ReadSigned("targets").at("targets").at(targetPath);
    auto target = SafeRelative(targetPath);
    auto custom = file.at("custom"), identity = custom.at("nativeIdentity");
    Require(custom.at("packageType") == "msix", ErrorCode::invalidInput,
            "App Installer export requires an MSIX target");
    auto base = BaseUrl(baseUrl);
    auto package = "targets/" + (target.parent_path() / (file.at("hashes").at("sha256").get<std::string>() +
                                                         "." + target.filename().string()))
                                    .generic_string();
    auto version = identity.at("version").get<std::string>();
    auto xml = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<AppInstaller "
               "xmlns=\"http://schemas.microsoft.com/appx/appinstaller/2021\" Version=\"" +
               Xml(version) + "\" Uri=\"" + Xml(base + output.filename().string()) +
               "\">\n  <MainPackage Name=\"" + Xml(identity.at("name")) + "\" Publisher=\"" +
               Xml(identity.at("publisher")) + "\" Version=\"" + Xml(version) +
               "\" ProcessorArchitecture=\"" + Xml(identity.at("processorArchitecture")) + "\" Uri=\"" +
               Xml(base + package) +
               "\" />\n  <UpdateSettings><OnLaunch HoursBetweenUpdateChecks=\"12\" "
               "/></UpdateSettings>\n</AppInstaller>\n";
    AtomicWrite(output, xml);
    return {{"status", "exported"}, {"format", "appinstaller"}, {"tufEnforcedByNativeFeed", false}};
}
} // namespace baz
