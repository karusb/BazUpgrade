#include "tuf/TufClient.hpp"
#include "storage/FileLock.hpp"
#include <algorithm>
#include <codecvt>
#include <locale>

namespace baz
{
namespace
{
std::uint64_t ReadMetadataVersion(const Json &value)
{
    const auto &number = value.at("version");

    Require(number.is_number_unsigned() || (number.is_number_integer() && number.get<std::int64_t>() > 0),
            ErrorCode::trust, "Invalid metadata version");
    auto result = number.get<std::uint64_t>();
    Require(result > 0 && result <= 9007199254740991ULL, ErrorCode::trust, "Invalid metadata version");

    return result;
}

bool MatchesDelegatedPath(const std::string &encodedPattern, const std::string &encodedValue)
{
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
    const auto pattern = utf8.from_bytes(encodedPattern);
    const auto value = utf8.from_bytes(encodedValue);

    auto matchesPatternToken = [&](std::size_t &position, char32_t character)
    {
        auto begin = position++;
        if (pattern[begin] == '?')
            return true;
        if (pattern[begin] != '[')
            return pattern[begin] == character;

        auto end = position;
        bool negate = end < pattern.size() && pattern[end] == '!';
        if (negate)
            ++end;
        auto first = end;
        if (end < pattern.size() && pattern[end] == ']')
            ++end;
        while (end < pattern.size() && pattern[end] != ']')
            ++end;
        if (end == pattern.size())
            return character == '[';

        bool matched = false;
        for (auto index = first; index < end; ++index)
        {
            if (index + 2 < end && pattern[index + 1] == '-')
            {
                matched = matched || (pattern[index] <= character && character <= pattern[index + 2]);
                index += 2;
            }
            else
                matched = matched || pattern[index] == character;
        }

        position = end + 1;

        return matched != negate;
    };

    std::size_t p = 0, v = 0, star = std::string::npos, saved = 0;
    while (v < value.size())
    {
        if (p < pattern.size() && pattern[p] == '*')
        {
            star = p++;
            saved = v;
            continue;
        }
        auto next = p;
        if (p < pattern.size() && matchesPatternToken(next, value[v]))
        {
            p = next;
            ++v;
            continue;
        }
        if (star == std::string::npos)
            return false;
        p = star + 1;
        v = ++saved;
    }
    while (p < pattern.size() && pattern[p] == '*')
        ++p;

    return p == pattern.size();
}
} // namespace

TufClient::TufClient(Transport &source, const CryptoProvider &verifier, fs::path trustedRoot,
                     fs::path directory)
    : transport(source), crypto(verifier), bootstrap(std::move(trustedRoot)), state(std::move(directory))
{
}

void VerifyThreshold(const CryptoProvider &crypto, const Json &envelope, const Json &keys, const Json &role)
{
    Require(role.at("threshold").is_number_integer(), ErrorCode::trust, "Invalid signature threshold");
    auto threshold = role.at("threshold").get<int>();
    std::set<std::string> authorized;
    for (const auto &id : role.at("keyids"))
        authorized.insert(id.get<std::string>());
    Require(threshold > 0 && static_cast<std::size_t>(threshold) <= authorized.size(), ErrorCode::trust,
            "Impossible signature threshold");
    auto message = CanonicalJson(envelope.at("signed"));
    std::set<std::string> valid;
    Require(envelope.at("signatures").is_array() && envelope.at("signatures").size() <= 128, ErrorCode::trust,
            "Invalid signature list");
    for (const auto &signature : envelope.at("signatures"))
    {
        auto id = signature.at("keyid").get<std::string>();
        if (authorized.count(id) && keys.contains(id) && !valid.count(id))
        {
            try
            {
                if (crypto.Verify(keys.at(id), message, signature.at("sig").get<std::string>()))
                    valid.insert(id);
            }
            catch (const Error &)
            { /* One invalid signature cannot satisfy a threshold. */
            }
        }
    }
    Require(valid.size() >= static_cast<std::size_t>(threshold), ErrorCode::trust,
            "Signature threshold not met");
}

void TufClient::VerifyMetadata(const Json &envelope, const std::string &type, const std::string &cacheName)
{
    const auto &body = envelope.at("signed");
    Require(body.at("_type") == type && body.at("spec_version").get<std::string>().rfind("1.0.", 0) == 0,
            ErrorCode::trust, "Unsupported metadata format");
    auto version = ReadMetadataVersion(body);
    Require(ParseUtc(body.at("expires")) > started, ErrorCode::expired, "Expired " + type + " metadata");
    auto cache = state / cacheName;
    if (fs::exists(cache))
    {
        auto old = ReadJson(cache).at("signed");
        Require(version >= ReadMetadataVersion(old), ErrorCode::rollback, "Metadata rollback detected");
        if (version == ReadMetadataVersion(old))
            Require(CanonicalJson(body) == CanonicalJson(old), ErrorCode::trust,
                    "Metadata changed without a version increment");
    }
}

void TufClient::VerifyMetadataDescription(const std::string &bytes, const Json &description) const
{
    if (description.contains("length"))
        Require(description.at("length") == bytes.size(), ErrorCode::trust, "Metadata size mismatch");
    if (description.contains("hashes"))
    {
        bool verified = false;
        for (auto it = description.at("hashes").begin(); it != description.at("hashes").end(); ++it)
            if (it.key() == "sha256" || it.key() == "sha512")
            {
                Require(crypto.Hash(bytes, it.key()) == it.value(), ErrorCode::trust,
                        "Metadata hash mismatch");
                verified = true;
            }
        Require(verified, ErrorCode::trust, "No supported metadata hash");
    }
}

Json TufClient::FetchMetadata(const std::string &filename, const Json *description,
                              const Cancellation &cancel)
{
    std::string remote = filename;
    std::uint64_t limit = 4 * 1024 * 1024;

    if (description)
    {
        auto version = ReadMetadataVersion(*description);
        if (root.at("signed").value("consistent_snapshot", false))
            remote = std::to_string(version) + "." + filename;
        if (description->contains("length"))
            limit = std::min(limit, description->at("length").get<std::uint64_t>());
    }

    auto bytes = transport.Fetch("metadata/" + remote, limit, cancel);
    Require(bytes.has_value(), ErrorCode::transport, "Required metadata is missing: " + filename);

    if (description)
        VerifyMetadataDescription(*bytes, *description);

    auto envelope = ParseJson(*bytes);
    if (description)
        Require(ReadMetadataVersion(envelope.at("signed")) == ReadMetadataVersion(*description),
                ErrorCode::trust, "Metadata version mismatch");

    return envelope;
}

void TufClient::RefreshTrustedMetadata(const Cancellation &cancel)
{
    started = ParseUtc(UtcNow());
    auto initial = ReadJson(bootstrap);
    auto fingerprint = crypto.Hash(CanonicalJson(initial));

    if (fs::exists(state / "bootstrap.json"))
        Require(ReadJson(state / "bootstrap.json").at("sha256") == fingerprint, ErrorCode::trust,
                "State belongs to another trust bootstrap");
    else
        WriteJson(state / "bootstrap.json", {{"sha256", fingerprint}});

    if (fs::exists(state / "root.json"))
        root = ReadJson(state / "root.json");
    else
    {
        root = initial;
        VerifyThreshold(crypto, root, root.at("signed").at("keys"), root.at("signed").at("roles").at("root"));
        Require(root.at("signed").at("_type") == "root", ErrorCode::trust, "Expected trusted root");
        WriteJson(state / "root.json", root);
    }

    Require(root.at("signed").at("_type") == "root" &&
                root.at("signed").at("spec_version").get<std::string>().rfind("1.0.", 0) == 0,
            ErrorCode::trust, "Invalid cached root format");
    VerifyThreshold(crypto, root, root.at("signed").at("keys"), root.at("signed").at("roles").at("root"));

    for (unsigned count = 0;; ++count)
    {
        auto nextVersion = ReadMetadataVersion(root.at("signed")) + 1;
        auto bytes = transport.Fetch("metadata/" + std::to_string(nextVersion) + ".root.json",
                                     4 * 1024 * 1024, cancel);

        if (!bytes)
            break;

        Require(count < 32, ErrorCode::trust, "Too many root rotations in one cycle");
        auto next = ParseJson(*bytes);
        VerifyThreshold(crypto, next, root.at("signed").at("keys"), root.at("signed").at("roles").at("root"));
        VerifyThreshold(crypto, next, next.at("signed").at("keys"), next.at("signed").at("roles").at("root"));
        Require(ReadMetadataVersion(next.at("signed")) == nextVersion &&
                    next.at("signed").at("_type") == "root",
                ErrorCode::trust, "Invalid root rotation");
        Require(next.at("signed").at("spec_version").get<std::string>().rfind("1.0.", 0) == 0,
                ErrorCode::trust, "Unsupported root format");

        for (const std::string role : {"timestamp", "snapshot"})
        {
            const auto &oldRole = root.at("signed").at("roles").at(role);
            const auto &newRole = next.at("signed").at("roles").at(role);
            bool changed = oldRole != newRole;
            for (const auto &key : oldRole.at("keyids"))
                if (!next.at("signed").at("keys").contains(key.get<std::string>()) ||
                    root.at("signed").at("keys").at(key.get<std::string>()) !=
                        next.at("signed").at("keys").at(key.get<std::string>()))
                    changed = true;

            if (changed)
                fs::remove(state / (role + ".json"));
        }

        root = next;
        WriteJson(state / "root.json", root);
    }

    Require(ParseUtc(root.at("signed").at("expires")) > started, ErrorCode::expired,
            "Trusted root expired; explicit out-of-band recovery is required");

    const auto &authority = root.at("signed");
    auto timestamp = FetchMetadata("timestamp.json", nullptr, cancel);
    VerifyThreshold(crypto, timestamp, authority.at("keys"), authority.at("roles").at("timestamp"));
    VerifyMetadata(timestamp, "timestamp", "timestamp.json");
    auto snapshotDescription = timestamp.at("signed").at("meta").at("snapshot.json");
    if (fs::exists(state / "timestamp.json"))
        Require(ReadMetadataVersion(snapshotDescription) >=
                    ReadMetadataVersion(
                        ReadJson(state / "timestamp.json").at("signed").at("meta").at("snapshot.json")),
                ErrorCode::rollback, "Snapshot reference rollback");
    WriteJson(state / "timestamp.json", timestamp);

    snapshot = FetchMetadata("snapshot.json", &snapshotDescription, cancel);
    VerifyThreshold(crypto, snapshot, authority.at("keys"), authority.at("roles").at("snapshot"));
    VerifyMetadata(snapshot, "snapshot", "snapshot.json");
    if (fs::exists(state / "snapshot.json"))
    {
        auto previous = ReadJson(state / "snapshot.json").at("signed").at("meta");
        for (auto it = previous.begin(); it != previous.end(); ++it)
            Require(snapshot.at("signed").at("meta").contains(it.key()) &&
                        ReadMetadataVersion(snapshot.at("signed").at("meta").at(it.key())) >=
                            ReadMetadataVersion(it.value()),
                    ErrorCode::rollback, "Targets reference removed or rolled back");
    }
    WriteJson(state / "snapshot.json", snapshot);

    targets = FetchMetadata("targets.json", &snapshot.at("signed").at("meta").at("targets.json"), cancel);
    VerifyThreshold(crypto, targets, authority.at("keys"), authority.at("roles").at("targets"));
    VerifyMetadata(targets, "targets", "targets.json");
    WriteJson(state / "targets.json", targets);
}

std::optional<Target> TufClient::FindDelegatedTarget(const Json &metadata, const std::string &path,
                                                     const Cancellation &cancel,
                                                     std::set<std::string> &visited, unsigned depth,
                                                     bool &terminated)
{
    Require(depth <= 32 && delegatedCount <= 128, ErrorCode::trust, "Delegation traversal limit exceeded");
    const auto &body = metadata.at("signed");

    if (body.at("targets").contains(path))
    {
        const auto &file = body.at("targets").at(path);
        Require(file.at("length").is_number_unsigned() ||
                    (file.at("length").is_number_integer() && file.at("length").get<std::int64_t>() >= 0),
                ErrorCode::trust, "Invalid target size");
        Require(file.at("hashes").contains("sha256"), ErrorCode::unsupported,
                "This profile requires SHA-256 targets");
        return Target{path, file.at("length").get<std::uint64_t>(), file.at("hashes"),
                      file.value("custom", Json::object())};
    }

    if (!body.contains("delegations"))
        return std::nullopt;

    const auto &delegations = body.at("delegations");
    Require(!delegations.contains("succinct_roles"), ErrorCode::unsupported,
            "Succinct delegations are not supported by this profile");

    for (const auto &role : delegations.at("roles"))
    {
        bool match = false;

        Require(role.contains("paths") != role.contains("path_hash_prefixes"), ErrorCode::trust,
                "Ambiguous delegation scope");
        if (role.contains("paths"))
            for (const auto &pattern : role.at("paths"))
                match = match || MatchesDelegatedPath(pattern, path);
        else
        {
            auto hash = crypto.Hash(path);
            for (const auto &prefix : role.at("path_hash_prefixes"))
                match = match || hash.rfind(prefix.get<std::string>(), 0) == 0;
        }

        if (!match)
            continue;

        auto name = role.at("name").get<std::string>();
        SafeRelative(name);
        Require(name.find('/') == std::string::npos && name != "root" && name != "snapshot" &&
                    name != "timestamp" && name != "targets",
                ErrorCode::trust, "Unsafe delegated role name");

        if (visited.insert(name).second)
        {
            ++delegatedCount;
            const auto &description = snapshot.at("signed").at("meta").at(name + ".json");
            auto child = FetchMetadata(name + ".json", &description, cancel);

            VerifyThreshold(crypto, child, delegations.at("keys"), role);
            auto cacheName = "delegated/" + crypto.Hash(name) + ".json";
            VerifyMetadata(child, "targets", cacheName);
            WriteJson(state / cacheName, child);

            auto found = FindDelegatedTarget(child, path, cancel, visited, depth + 1, terminated);
            if (found)
                return found;
            if (terminated)
                return std::nullopt;
        }

        if (role.at("terminating").get<bool>())
        {
            terminated = true;
            return std::nullopt;
        }
    }

    return std::nullopt;
}

Target TufClient::ResolveTarget(const std::string &path, const Cancellation &cancel)
{
    SafeRelative(path);

    FileLock lock(state / "trust.lock");

    RefreshTrustedMetadata(cancel);

    delegatedCount = 0;
    std::set<std::string> visited{"targets"};
    bool terminated = false;

    auto target = FindDelegatedTarget(targets, path, cancel, visited, 0, terminated);
    Require(target.has_value(), ErrorCode::trust, "Target is not authorized by trusted metadata");

    return *target;
}

void TufClient::DownloadAndVerifyTarget(const Target &target, const fs::path &destination,
                                        const Cancellation &cancel)
{
    auto path = SafeRelative(target.path);
    if (root.at("signed").value("consistent_snapshot", false))
        path = path.parent_path() /
               (target.hashes.at("sha256").get<std::string>() + "." + path.filename().string());

    transport.Download("targets/" + path.generic_string(), destination, target.length, cancel);

    Require(fs::file_size(destination) == target.length, ErrorCode::trust, "Target size mismatch");
    for (auto it = target.hashes.begin(); it != target.hashes.end(); ++it)
        if (it.key() == "sha256" || it.key() == "sha512")
            Require(crypto.HashFile(destination, it.key()) == it.value(), ErrorCode::trust,
                    "Target hash mismatch");
}
} // namespace baz
