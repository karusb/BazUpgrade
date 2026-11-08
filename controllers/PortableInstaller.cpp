#include "controllers/PortableInstaller.hpp"
#include <algorithm>
#include <archive.h>
#include <archive_entry.h>
#include <fstream>
#include <memory>
#include <set>
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

InstallResult PortableInstaller::Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                                       const Cancellation &cancel)
{
    auto hash = Component(target.hashes.at("sha256"));
    auto versionDirectory = root / "versions" / hash;
    SecureDirectory(root / "versions");
    auto entrypoint = SafeRelative(target.custom.at("entrypoint"));
    if (fs::exists(versionDirectory))
    {
        SecureDirectory(versionDirectory);
        auto marker = versionDirectory / ".bazupgrade-version.json";
        Require(fs::is_regular_file(marker) && !fs::is_symlink(marker) &&
                    ReadJson(marker).at("sha256") == hash &&
                    fs::is_regular_file(versionDirectory / entrypoint),
                ErrorCode::io, "Version staging is incomplete; inspect and reconcile");
        return {false, (versionDirectory / entrypoint).generic_u8string()};
    }
    SecureDirectory(versionDirectory);
    std::unique_ptr<archive, decltype(&archive_read_free)> input(archive_read_new(), archive_read_free);
    Require(input != nullptr, ErrorCode::io, "Cannot allocate archive reader");
    archive_read_support_filter_none(input.get());
    archive_read_support_filter_gzip(input.get());
    archive_read_support_format_zip(input.get());
    archive_read_support_format_tar(input.get());
#ifdef _WIN32
    auto opened = archive_read_open_filename_w(input.get(), artifact.c_str(), 65536);
#else
    auto opened = archive_read_open_filename(input.get(), artifact.c_str(), 65536);
#endif
    Require(opened == ARCHIVE_OK, ErrorCode::invalidInput, "Cannot open portable archive");
    archive_entry *entry = nullptr;
    std::uint64_t entries = 0, total = 0;
    std::set<std::string> paths;
    int status = ARCHIVE_OK;
    while ((status = archive_read_next_header(input.get(), &entry)) == ARCHIVE_OK)
    {
        cancel.Check();
        Require(++entries <= maxEntries, ErrorCode::invalidInput, "Archive entry quota exceeded");
        Require(!archive_entry_symlink(entry) && !archive_entry_hardlink(entry), ErrorCode::invalidInput,
                "Archive links are forbidden");
        const char *raw = archive_entry_pathname_utf8(entry);
        if (!raw)
            raw = archive_entry_pathname(entry);
        Require(raw != nullptr, ErrorCode::invalidInput, "Missing archive pathname");
        std::string name(raw);
        if (!name.empty() && name.back() == '/')
            name.pop_back();
        Require(name != ".bazupgrade-version.json", ErrorCode::invalidInput, "Reserved archive marker");
        auto relative = SafeRelative(name);
        std::string folded = relative.generic_u8string();
        std::transform(folded.begin(), folded.end(), folded.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        Require(paths.insert(folded).second, ErrorCode::invalidInput,
                "Duplicate or case-colliding archive path");
        auto output = versionDirectory / relative;
        auto type = archive_entry_filetype(entry);
        Require(type == AE_IFREG || type == AE_IFDIR, ErrorCode::invalidInput,
                "Archive special files are forbidden");
        SecureDirectory(output.parent_path());
        if (type == AE_IFDIR)
        {
            SecureDirectory(output);
            continue;
        }
        Require(!fs::exists(output), ErrorCode::invalidInput,
                "Archive entry would overwrite an existing file");
        Require(archive_entry_size(entry) >= 0 &&
                    static_cast<std::uint64_t>(archive_entry_size(entry)) <= maxBytes - total,
                ErrorCode::invalidInput, "Archive size quota exceeded");
        std::ofstream file(output, std::ios::binary);
        Require(file.good(), ErrorCode::io, "Cannot create extracted file");
        char buffer[65536];
        la_ssize_t received = 0;
        while ((received = archive_read_data(input.get(), buffer, sizeof(buffer))) > 0)
        {
            cancel.Check();
            Require(static_cast<std::uint64_t>(received) <= maxBytes - total, ErrorCode::invalidInput,
                    "Archive size quota exceeded");
            total += static_cast<std::uint64_t>(received);
            file.write(buffer, received);
            Require(file.good(), ErrorCode::io, "Archive write failed");
        }
        Require(received == 0, ErrorCode::invalidInput, "Corrupt archive entry");
        file.close();
        Require(file.good(), ErrorCode::io, "Cannot finish extracted file");
#ifndef _WIN32
        auto mode = archive_entry_perm(entry);
        fs::permissions(output, (mode & 0111)
                                    ? fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec |
                                          fs::perms::others_read | fs::perms::others_exec
                                    : fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read |
                                          fs::perms::others_read);
#endif
        FlushFile(output);
    }
    Require(status == ARCHIVE_EOF && entries > 0, ErrorCode::invalidInput,
            "Invalid or empty portable archive");
    Require(fs::is_regular_file(versionDirectory / entrypoint), ErrorCode::invalidInput,
            "Package entrypoint is missing");
    for (const auto &item : fs::recursive_directory_iterator(versionDirectory))
        if (item.is_directory())
            FlushDirectory(item.path());
    FlushDirectory(versionDirectory);
    FlushDirectory(root / "versions");
    FlushDirectory(root);
    WriteJson(versionDirectory / ".bazupgrade-version.json", {{"sha256", hash}});
    return {false, (versionDirectory / entrypoint).generic_u8string()};
}

void PackDirectory(const fs::path &directory, const fs::path &output)
{
    auto source = fs::canonical(directory);
    Require(!fs::exists(output) && !fs::is_symlink(fs::symlink_status(output)), ErrorCode::invalidInput,
            "Refusing to overwrite an archive");
    auto absoluteOutput = fs::absolute(output).lexically_normal();
    auto relativeOutput = absoluteOutput.lexically_relative(source);
    Require(relativeOutput.empty() || *relativeOutput.begin() == "..", ErrorCode::invalidInput,
            "Archive output must be outside its source directory");
    std::unique_ptr<archive, decltype(&archive_write_free)> writer(archive_write_new(), archive_write_free);
    archive_write_set_format_zip(writer.get());
#ifdef _WIN32
    auto opened = archive_write_open_filename_w(writer.get(), output.c_str());
#else
    auto opened = archive_write_open_filename(writer.get(), output.c_str());
#endif
    Require(opened == ARCHIVE_OK, ErrorCode::io, "Cannot create archive");
    std::vector<fs::path> files;
    for (const auto &entry : fs::recursive_directory_iterator(source))
    {
        Require(!entry.is_symlink(), ErrorCode::invalidInput, "Package source contains a symlink");
        if (entry.is_regular_file())
            files.push_back(entry.path());
        else
            Require(entry.is_directory(), ErrorCode::invalidInput, "Package source contains a special file");
    }
    std::sort(files.begin(), files.end());
    Require(!files.empty(), ErrorCode::invalidInput, "Package source is empty");
    for (const auto &path : files)
    {
        auto relative = fs::relative(path, source).generic_u8string();
        SafeRelative(relative);
        std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entry(archive_entry_new(),
                                                                            archive_entry_free);
        archive_entry_set_pathname_utf8(entry.get(), relative.c_str());
        archive_entry_set_filetype(entry.get(), AE_IFREG);
        archive_entry_set_size(entry.get(), static_cast<la_int64_t>(fs::file_size(path)));
        archive_entry_set_perm(
            entry.get(),
            (fs::status(path).permissions() & fs::perms::owner_exec) != fs::perms::none ? 0755 : 0644);
        Require(archive_write_header(writer.get(), entry.get()) == ARCHIVE_OK, ErrorCode::io,
                "Cannot write archive entry");
        std::ifstream input(path, std::ios::binary);
        char buffer[65536];
        while (input.read(buffer, sizeof(buffer)) || input.gcount())
            Require(archive_write_data(writer.get(), buffer, static_cast<std::size_t>(input.gcount())) ==
                        input.gcount(),
                    ErrorCode::io, "Archive write failed");
        Require(input.eof(), ErrorCode::io, "Package source read failed");
    }
    Require(archive_write_close(writer.get()) == ARCHIVE_OK, ErrorCode::io, "Cannot finish archive");
}
} // namespace baz
