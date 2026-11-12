#include "controllers/PortableInstaller.hpp"
#include "controllers/detail/PortableArchiveReader.hpp"
#include "controllers/detail/PortableArchiveWriter.hpp"
#ifdef _WIN32
#include <windows.h>
#endif

namespace baz
{
void SecureDirectory(const fs::path &directory)
{
    fs::path current;
    for (const auto &part : fs::absolute(directory).lexically_normal())
    {
        current /= part;
        if (fs::exists(current))
        {
            Require(!fs::is_symlink(fs::symlink_status(current)), ErrorCode::io,
                    "Managed directories must not contain symlinks");
#ifdef _WIN32
            auto attributes = GetFileAttributesW(current.c_str());
            Require(attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT),
                    ErrorCode::io, "Managed directories must not contain reparse points");
#endif
            Require(fs::is_directory(current), ErrorCode::io, "Managed directory path contains a file");
        }
        else
            fs::create_directory(current);
    }
}

PortableInstaller::PortableInstaller(std::uint64_t bytes, std::uint64_t entries)
    : maxBytes(bytes), maxEntries(entries)
{
}

void PortableInstaller::ValidateCompletedVersion(const fs::path &directory, const fs::path &entrypoint,
                                                 const std::string &hash)
{
    const auto marker = directory / ".bazupgrade-version.json";

    SecureDirectory(directory);
    Require(fs::is_regular_file(marker) && !fs::is_symlink(marker) && ReadJson(marker).at("sha256") == hash &&
                fs::is_regular_file(directory / entrypoint),
            ErrorCode::io, "Version staging is incomplete; inspect and reconcile");
}

void PortableInstaller::CommitExtractedVersion(const fs::path &root, const fs::path &directory,
                                               const fs::path &entrypoint, const std::string &hash)
{
    Require(fs::is_regular_file(directory / entrypoint), ErrorCode::invalidInput,
            "Package entrypoint is missing");

    for (const auto &item : fs::recursive_directory_iterator(directory))
        if (item.is_directory())
            FlushDirectory(item.path());

    FlushDirectory(directory);
    FlushDirectory(root / "versions");
    FlushDirectory(root);
    WriteJson(directory / ".bazupgrade-version.json", {{"sha256", hash}});
}

InstallResult PortableInstaller::Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                                       const Cancellation &cancel)
{
    const auto hash = Component(target.hashes.at("sha256"));
    const auto directory = root / "versions" / hash;

    SecureDirectory(root / "versions");
    const auto entrypoint = SafeRelative(target.custom.at("entrypoint"));

    if (fs::exists(directory))
    {
        ValidateCompletedVersion(directory, entrypoint, hash);

        return {false, (directory / entrypoint).generic_u8string()};
    }

    SecureDirectory(directory);
    detail::PortableArchiveReader reader(maxBytes, maxEntries, cancel);

    reader.ExtractTo(artifact, directory);
    CommitExtractedVersion(root, directory, entrypoint, hash);

    return {false, (directory / entrypoint).generic_u8string()};
}

void PackDirectory(const fs::path &directory, const fs::path &output)
{
    detail::PortableArchiveWriter writer;

    writer.WriteDirectory(directory, output);
}
} // namespace baz
