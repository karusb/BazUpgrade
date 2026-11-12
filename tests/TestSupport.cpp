#include "tests/TestSupport.hpp"
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <fstream>
#include <memory>
#include <openssl/rand.h>
#ifdef _WIN32
#include <windows.h>
#include <winioctl.h>
#endif

namespace test
{
void WriteFixtureFile(const fs::path &path, const std::string &bytes)
{
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);

    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    Require(output.good(), ErrorCode::io, "Fixture write failed");
}

TemporaryDirectory::TemporaryDirectory()
{
    unsigned char random[16];

    Require(RAND_bytes(random, sizeof(random)) == 1, ErrorCode::io, "Random fixture name failed");
    directory = fs::canonical(fs::temp_directory_path()) / ("bazupgrade-test-" + Hex(random, sizeof(random)));
    Require(fs::create_directory(directory), ErrorCode::io, "Fixture directory already exists");
#ifndef _WIN32
    fs::permissions(directory, fs::perms::owner_all);
#endif
}

TemporaryDirectory::~TemporaryDirectory()
{
    std::error_code ignored;

    fs::remove_all(directory, ignored);
}

const fs::path &TemporaryDirectory::Path() const noexcept
{
    return directory;
}

RepositoryFixture::RepositoryFixture()
    : root(workspace.Path()), repo(root / "repository"), keys(root / "private"),
      bootstrap(root / "bootstrap.json"), package(root / "application.zip")
{
    RepositoryPublisher(repo, keys).Initialize();
    fs::copy_file(repo / "metadata/1.root.json", bootstrap);

    WriteApplicationArchive("synthetic version one");
    ConfigureClient();
}

void RepositoryFixture::ConfigureClient()
{
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

Json RepositoryFixture::BuildReleaseDescriptor(std::uint64_t sequence) const
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

void RepositoryFixture::PublishApplicationRelease(std::uint64_t sequence)
{
    RepositoryPublisher(repo, keys).Publish(package, config.target, BuildReleaseDescriptor(sequence));
}

void RepositoryFixture::WriteApplicationArchive(const std::string &contents)
{
    WriteFixtureFile(root / "application/app.txt", contents);
    fs::remove(package);

    PackDirectory(root / "application", package);
}

std::vector<fs::path> RepositoryFixture::GetRoleSigningKeys(const std::string &role) const
{
    const auto ring = ReadJson(keys / "keyring.json");
    std::vector<fs::path> files;

    for (const auto &name : ring.at("privateKeys").at(role))
        files.push_back(keys / name.get<std::string>());

    return files;
}

void RepositoryFixture::WriteSignedMetadata(const std::string &filename, const Json &body,
                                            const std::string &role)
{
    WriteJson(repo / "metadata" / filename, Sign(body, GetRoleSigningKeys(role)));
}

Json RepositoryFixture::DescribeMetadata(const Json &envelope) const
{
    const auto bytes = envelope.dump(2) + '\n';

    return {{"version", envelope.at("signed").at("version")},
            {"length", bytes.size()},
            {"hashes", {{"sha256", crypto.Hash(bytes)}}}};
}

void RepositoryFixture::WriteVersionedMetadata(const std::string &role, const Json &envelope)
{
    const auto version = envelope.at("signed").at("version").get<std::uint64_t>();
    const auto filename = std::to_string(version) + "." + role + ".json";

    WriteJson(repo / "metadata" / (role + ".json"), envelope);
    WriteJson(repo / "metadata" / filename, envelope);
}

void RepositoryFixture::AddDelegatedRole(const std::string &name, const std::string &pattern,
                                         bool terminating, bool includeTarget)
{
    const auto privateKey = keys / (name + ".pem");
    const auto key = GenerateEd25519Key(privateKey);
    const auto id = crypto.Hash(CanonicalJson(key));
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

    UpdateDelegationMetadataChain(name, parent, Sign(child, {privateKey}));
}

void RepositoryFixture::UpdateDelegationMetadataChain(const std::string &name, const Json &parent,
                                                      const Json &child)
{
    auto snapshot = ReadJson(repo / "metadata/snapshot.json").at("signed");
    auto timestamp = ReadJson(repo / "metadata/timestamp.json").at("signed");
    const auto signedParent = Sign(parent, GetRoleSigningKeys("targets"));

    WriteJson(repo / "metadata" / ("1." + name + ".json"), child);
    WriteVersionedMetadata("targets", signedParent);

    snapshot["meta"][name + ".json"] = DescribeMetadata(child);
    snapshot["meta"]["targets.json"] = DescribeMetadata(signedParent);
    const auto signedSnapshot = Sign(snapshot, GetRoleSigningKeys("snapshot"));

    WriteVersionedMetadata("snapshot", signedSnapshot);

    timestamp["meta"]["snapshot.json"] = DescribeMetadata(signedSnapshot);
    WriteSignedMetadata("timestamp.json", timestamp, "timestamp");
}

void RepositoryFixture::CreateUnsafeArchive(const std::string &name, bool symlink)
{
    std::unique_ptr<archive, decltype(&archive_write_free)> writer(archive_write_new(), archive_write_free);
    std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entry(archive_entry_new(),
                                                                        archive_entry_free);

    Require(writer && entry, ErrorCode::io, "Cannot allocate fixture archive");
    fs::remove(package);
    archive_write_set_format_zip(writer.get());
    Require(archive_write_open_filename(writer.get(), package.string().c_str()) == ARCHIVE_OK, ErrorCode::io,
            "Fixture archive open failed");

    archive_entry_set_pathname(entry.get(), name.c_str());
    archive_entry_set_filetype(entry.get(), symlink ? AE_IFLNK : AE_IFREG);
    archive_entry_set_perm(entry.get(), 0644);
    archive_entry_set_size(entry.get(), symlink ? 0 : 4);
    if (symlink)
        archive_entry_set_symlink(entry.get(), "../outside");

    Require(archive_write_header(writer.get(), entry.get()) == ARCHIVE_OK, ErrorCode::io,
            "Fixture archive header failed");
    if (!symlink)
        Require(archive_write_data(writer.get(), "test", 4) == 4, ErrorCode::io,
                "Fixture archive write failed");

    Require(archive_write_close(writer.get()) == ARCHIVE_OK, ErrorCode::io, "Fixture archive close failed");
}
#ifdef _WIN32
void RepositoryFixture::ReplaceMetadataWithJunctionTo(const fs::path &destination)
{
    const auto target = L"\\??\\" + destination.wstring();
    const auto display = destination.wstring();

    struct Junction
    {
        DWORD tag;
        USHORT length;
        USHORT reserved;
        USHORT substituteOffset;
        USHORT substituteLength;
        USHORT printOffset;
        USHORT printLength;
    };

    const auto pathBytes = (target.size() + display.size() + 2) * sizeof(wchar_t);
    std::vector<BYTE> buffer(sizeof(Junction) + pathBytes);
    auto junction = reinterpret_cast<Junction *>(buffer.data());

    for (const auto &entry : fs::directory_iterator(repo / "metadata"))
        fs::remove(entry.path());

    junction->tag = IO_REPARSE_TAG_MOUNT_POINT;
    junction->length = static_cast<USHORT>(8 + pathBytes);
    junction->substituteLength = static_cast<USHORT>(target.size() * sizeof(wchar_t));
    junction->printOffset = static_cast<USHORT>((target.size() + 1) * sizeof(wchar_t));
    junction->printLength = static_cast<USHORT>(display.size() * sizeof(wchar_t));
    auto paths = reinterpret_cast<wchar_t *>(buffer.data() + sizeof(Junction));

    std::copy(target.begin(), target.end(), paths);
    std::copy(display.begin(), display.end(), paths + target.size() + 1);

    const auto handle =
        CreateFileW((repo / "metadata").c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    Require(handle != INVALID_HANDLE_VALUE, ErrorCode::io, "Cannot create isolated junction fixture");

    DWORD returned = 0;
    const auto created = DeviceIoControl(handle, FSCTL_SET_REPARSE_POINT, buffer.data(),
                                         static_cast<DWORD>(buffer.size()), nullptr, 0, &returned, nullptr);

    CloseHandle(handle);
    Require(created != FALSE, ErrorCode::io, "Cannot set isolated junction fixture");
}
#endif
} // namespace test
