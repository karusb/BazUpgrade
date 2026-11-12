#include "controllers/detail/PortableArchiveReader.hpp"
#include "controllers/PortableInstaller.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>

namespace baz::detail
{
PortableArchiveReader::PortableArchiveReader(std::uint64_t bytes, std::uint64_t entries,
                                             const Cancellation &cancellation)
    : input(archive_read_new(), archive_read_free), maxBytes(bytes), maxEntries(entries), cancel(cancellation)
{
    Require(input != nullptr, ErrorCode::io, "Cannot allocate archive reader");

    archive_read_support_filter_none(input.get());
    archive_read_support_filter_gzip(input.get());
    archive_read_support_format_zip(input.get());
    archive_read_support_format_tar(input.get());
}

void PortableArchiveReader::OpenArchive(const fs::path &artifact)
{
#ifdef _WIN32
    const auto opened = archive_read_open_filename_w(input.get(), artifact.c_str(), 65536);
#else
    const auto opened = archive_read_open_filename(input.get(), artifact.c_str(), 65536);
#endif

    Require(opened == ARCHIVE_OK, ErrorCode::invalidInput, "Cannot open portable archive");
}

fs::path PortableArchiveReader::ReadRelativeEntryPath(archive_entry *entry)
{
    const char *raw = archive_entry_pathname_utf8(entry);

    if (!raw)
        raw = archive_entry_pathname(entry);
    Require(raw != nullptr, ErrorCode::invalidInput, "Missing archive pathname");

    std::string name(raw);
    if (!name.empty() && name.back() == '/')
        name.pop_back();
    Require(name != ".bazupgrade-version.json", ErrorCode::invalidInput, "Reserved archive marker");

    const auto relative = SafeRelative(name);
    auto folded = relative.generic_u8string();

    std::transform(folded.begin(), folded.end(), folded.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    Require(paths.insert(folded).second, ErrorCode::invalidInput, "Duplicate or case-colliding archive path");

    return relative;
}

void PortableArchiveReader::CopyEntryContents(std::ostream &output)
{
    char buffer[65536];
    la_ssize_t received = 0;

    while ((received = archive_read_data(input.get(), buffer, sizeof(buffer))) > 0)
    {
        cancel.Check();
        Require(static_cast<std::uint64_t>(received) <= maxBytes - receivedBytes, ErrorCode::invalidInput,
                "Archive size quota exceeded");

        receivedBytes += static_cast<std::uint64_t>(received);
        output.write(buffer, received);
        Require(output.good(), ErrorCode::io, "Archive write failed");
    }

    Require(received == 0, ErrorCode::invalidInput, "Corrupt archive entry");
}

void PortableArchiveReader::SetExtractedPermissions(archive_entry *entry, const fs::path &output)
{
#ifndef _WIN32
    const auto mode = archive_entry_perm(entry);
    const auto permissions =
        (mode & 0111)
            ? fs::perms::owner_all | fs::perms::group_read | fs::perms::group_exec | fs::perms::others_read |
                  fs::perms::others_exec
            : fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::others_read;

    fs::permissions(output, permissions);
#else
    (void)entry;
    (void)output;
#endif
}

void PortableArchiveReader::ExtractFile(archive_entry *entry, const fs::path &output)
{
    Require(!fs::exists(output), ErrorCode::invalidInput, "Archive entry would overwrite an existing file");
    Require(archive_entry_size(entry) >= 0 &&
                static_cast<std::uint64_t>(archive_entry_size(entry)) <= maxBytes - receivedBytes,
            ErrorCode::invalidInput, "Archive size quota exceeded");

    std::ofstream file(output, std::ios::binary);

    Require(file.good(), ErrorCode::io, "Cannot create extracted file");
    CopyEntryContents(file);
    file.close();
    Require(file.good(), ErrorCode::io, "Cannot finish extracted file");

    SetExtractedPermissions(entry, output);
    FlushFile(output);
}

void PortableArchiveReader::ExtractEntry(archive_entry *entry, const fs::path &destination)
{
    Require(++entryCount <= maxEntries, ErrorCode::invalidInput, "Archive entry quota exceeded");
    Require(!archive_entry_symlink(entry) && !archive_entry_hardlink(entry), ErrorCode::invalidInput,
            "Archive links are forbidden");

    const auto relative = ReadRelativeEntryPath(entry);
    const auto output = destination / relative;
    const auto type = archive_entry_filetype(entry);

    Require(type == AE_IFREG || type == AE_IFDIR, ErrorCode::invalidInput,
            "Archive special files are forbidden");
    SecureDirectory(output.parent_path());

    if (type == AE_IFDIR)
        SecureDirectory(output);
    else
        ExtractFile(entry, output);
}

void PortableArchiveReader::ExtractTo(const fs::path &artifact, const fs::path &destination)
{
    archive_entry *entry = nullptr;
    int status = ARCHIVE_OK;

    OpenArchive(artifact);

    while ((status = archive_read_next_header(input.get(), &entry)) == ARCHIVE_OK)
    {
        cancel.Check();
        ExtractEntry(entry, destination);
    }

    Require(status == ARCHIVE_EOF && entryCount > 0, ErrorCode::invalidInput,
            "Invalid or empty portable archive");
}
} // namespace baz::detail
