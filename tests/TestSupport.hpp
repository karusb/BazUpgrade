#pragma once
#include "controllers/PortableInstaller.hpp"
#include "controllers/UpdateClient.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include "curl/RepositoryTransport.hpp"
#include "publishing/RepositoryPublisher.hpp"
#include "storage/FileLock.hpp"
#include "tuf/Metadata.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <openssl/rand.h>

namespace test
{
using namespace baz;
extern unsigned passed, failed;

inline void Check(bool value, const std::string &message)
{
    if (!value)
        throw std::runtime_error(message);
}

inline void Case(const char *name, const std::function<void()> &body)
{
    try
    {
        body();
        ++passed;
        std::cout << "PASS " << name << '\n';
    }
    catch (const std::exception &error)
    {
        ++failed;
        std::cerr << "FAIL " << name << ": " << error.what() << '\n';
    }
}

inline void Reject(ErrorCode code, const std::function<void()> &body)
{
    try
    {
        body();
    }
    catch (const Error &error)
    {
        Check(error.code == code, std::string("Wrong rejection: ") + error.what());
        return;
    }
    throw std::runtime_error("Operation unexpectedly succeeded");
}

inline void Bytes(const fs::path &path, const std::string &value)
{
    fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << value;
    Check(file.good(), "Fixture write failed");
}

struct Fixture
{
    fs::path root, repo, keys, bootstrap, package;
    ClientConfig config;
    OpenSslCrypto crypto;
    Cancellation cancel;

    Fixture()
    {
        unsigned char random[16];
        Check(RAND_bytes(random, sizeof(random)) == 1, "Random fixture name failed");
        root = fs::canonical(fs::temp_directory_path()) / ("bazupgrade-test-" + Hex(random, sizeof(random)));
        Check(fs::create_directory(root), "Fixture directory already exists");
#ifndef _WIN32
        fs::permissions(root, fs::perms::owner_all);
#endif
        repo = root / "repository";
        keys = root / "private";
        bootstrap = root / "bootstrap.json";
        package = root / "application.zip";
        RepositoryPublisher(repo, keys).Initialize();
        fs::copy_file(repo / "metadata/1.root.json", bootstrap);
        Bytes(root / "application/app.txt", "synthetic version one");
        PackDirectory(root / "application", package);
        config.repository = repo.u8string();
        config.appId = "example";
        config.channel = "stable";
        config.os = "test";
        config.arch = "x64";
        config.packageType = "portable";
        config.target = "example.zip";
        config.trustedRoot = bootstrap;
        config.stateDirectory = root / "client";
        config.installDirectory = root / "installed";
    }

    ~Fixture()
    {
        std::error_code error;
        fs::remove_all(root, error);
    }

    Json Custom(std::uint64_t sequence = 1) const
    {
        return {{"schemaVersion", 1},
                {"appId", config.appId},
                {"channel", config.channel},
                {"os", config.os},
                {"arch", config.arch},
                {"packageType", config.packageType},
                {"appVersion", "1." + std::to_string(sequence) + ".0"},
                {"releaseSequence", sequence},
                {"entrypoint", "app.txt"},
                {"minUpdaterMajor", 2}};
    }

    void Publish(std::uint64_t sequence = 1)
    {
        RepositoryPublisher(repo, keys).Publish(package, config.target, Custom(sequence));
    }

    std::vector<fs::path> RoleKeys(const std::string &role) const
    {
        std::vector<fs::path> files;
        auto ring = ReadJson(keys / "keyring.json");
        for (const auto &value : ring.at("privateKeys").at(role))
            files.push_back(keys / value.get<std::string>());
        return files;
    }

    void Resign(const std::string &name, const Json &body, const std::string &role)
    {
        WriteJson(repo / "metadata" / name, Sign(body, RoleKeys(role)));
    }

    void Delegate(const std::string &name, const std::string &pattern, bool terminating, bool includeTarget)
    {
        auto privateKey = keys / (name + ".pem");
        auto key = GenerateEd25519Key(privateKey);
        auto id = crypto.Hash(CanonicalJson(key));
        auto parent = ReadJson(repo / "metadata/targets.json").at("signed");
        Json child = {{"_type", "targets"},
                      {"spec_version", "1.0.31"},
                      {"version", 1},
                      {"expires", "2099-01-01T00:00:00Z"},
                      {"targets", Json::object()}};
        if (includeTarget)
            child["targets"][config.target] = parent.at("targets").at(config.target);
        parent["targets"] = Json::object();
        if (!parent.contains("delegations"))
            parent["delegations"] = {{"keys", Json::object()}, {"roles", Json::array()}};
        parent["delegations"]["keys"][id] = key;
        parent["delegations"]["roles"].push_back({{"name", name},
                                                  {"keyids", Json::array({id})},
                                                  {"threshold", 1},
                                                  {"terminating", terminating},
                                                  {"paths", Json::array({pattern})}});
        auto envelope = Sign(child, {privateKey});
        WriteJson(repo / "metadata" / ("1." + name + ".json"), envelope);
        auto snapshot = ReadJson(repo / "metadata/snapshot.json").at("signed");
        snapshot["meta"][name + ".json"] = DescriptionFor(envelope);
        auto signedParent = Sign(parent, RoleKeys("targets"));
        WriteJson(repo / "metadata/targets.json", signedParent);
        WriteJson(repo / "metadata" / (std::to_string(parent.at("version").get<int>()) + ".targets.json"),
                  signedParent);
        snapshot["meta"]["targets.json"] = DescriptionFor(signedParent);
        auto signedSnapshot = Sign(snapshot, RoleKeys("snapshot"));
        WriteJson(repo / "metadata/snapshot.json", signedSnapshot);
        WriteJson(repo / "metadata" / (std::to_string(snapshot.at("version").get<int>()) + ".snapshot.json"),
                  signedSnapshot);
        auto timestamp = ReadJson(repo / "metadata/timestamp.json").at("signed");
        timestamp["meta"]["snapshot.json"] = DescriptionFor(signedSnapshot);
        Resign("timestamp.json", timestamp, "timestamp");
    }

    Json DescriptionFor(const Json &envelope) const
    {
        auto bytes = envelope.dump(2) + '\n';
        return {{"version", envelope.at("signed").at("version")},
                {"length", bytes.size()},
                {"hashes", {{"sha256", crypto.Hash(bytes)}}}};
    }
};

inline Json Description(const Json &envelope)
{
    OpenSslCrypto crypto;
    auto bytes = envelope.dump(2) + '\n';
    return {{"version", envelope.at("signed").at("version")},
            {"length", bytes.size()},
            {"hashes", {{"sha256", crypto.Hash(bytes)}}}};
}

inline void MaliciousZip(const fs::path &output, const std::string &name, bool link = false)
{
    archive *writer = archive_write_new();
    archive_write_set_format_zip(writer);
    Check(archive_write_open_filename(writer, output.string().c_str()) == ARCHIVE_OK,
          "Fixture archive open failed");
    archive_entry *entry = archive_entry_new();
    archive_entry_set_pathname(entry, name.c_str());
    archive_entry_set_filetype(entry, link ? AE_IFLNK : AE_IFREG);
    archive_entry_set_perm(entry, 0644);
    archive_entry_set_size(entry, link ? 0 : 4);
    if (link)
        archive_entry_set_symlink(entry, "../outside");
    Check(archive_write_header(writer, entry) == ARCHIVE_OK, "Fixture archive header failed");
    if (!link)
        Check(archive_write_data(writer, "test", 4) == 4, "Fixture archive write failed");
    archive_entry_free(entry);
    archive_write_close(writer);
    archive_write_free(writer);
}

void TrustTests();
void EngineTests();
} // namespace test
