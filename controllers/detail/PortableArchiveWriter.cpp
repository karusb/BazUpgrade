#include "controllers/detail/PortableArchiveWriter.hpp"
#include <algorithm>
#include <archive_entry.h>
#include <fstream>

namespace baz::detail
{
PortableArchiveWriter::PortableArchiveWriter() : writer(archive_write_new(), archive_write_free)
{
    Require(writer != nullptr, ErrorCode::io, "Cannot allocate archive writer");

    archive_write_set_format_zip(writer.get());
}

void PortableArchiveWriter::OpenArchive(const fs::path &output)
{
#ifdef _WIN32
    const auto opened = archive_write_open_filename_w(writer.get(), output.c_str());
#else
    const auto opened = archive_write_open_filename(writer.get(), output.c_str());
#endif

    Require(opened == ARCHIVE_OK, ErrorCode::io, "Cannot create archive");
}

std::vector<fs::path> PortableArchiveWriter::CollectPackageFiles(const fs::path &source)
{
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

    return files;
}

void PortableArchiveWriter::CopyFileContents(const fs::path &path)
{
    std::ifstream input(path, std::ios::binary);
    char buffer[65536];

    while (input.read(buffer, sizeof(buffer)) || input.gcount())
        Require(archive_write_data(writer.get(), buffer, static_cast<std::size_t>(input.gcount())) ==
                    input.gcount(),
                ErrorCode::io, "Archive write failed");

    Require(input.eof(), ErrorCode::io, "Package source read failed");
}

void PortableArchiveWriter::AppendFile(const fs::path &path, const fs::path &source)
{
    const auto relative = fs::relative(path, source).generic_u8string();
    std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entry(archive_entry_new(),
                                                                        archive_entry_free);

    SafeRelative(relative);
    Require(entry != nullptr, ErrorCode::io, "Cannot allocate archive entry");

    archive_entry_set_pathname_utf8(entry.get(), relative.c_str());
    archive_entry_set_filetype(entry.get(), AE_IFREG);
    archive_entry_set_size(entry.get(), static_cast<la_int64_t>(fs::file_size(path)));
    archive_entry_set_perm(
        entry.get(),
        (fs::status(path).permissions() & fs::perms::owner_exec) != fs::perms::none ? 0755 : 0644);
    Require(archive_write_header(writer.get(), entry.get()) == ARCHIVE_OK, ErrorCode::io,
            "Cannot write archive entry");

    CopyFileContents(path);
}

void PortableArchiveWriter::WriteDirectory(const fs::path &directory, const fs::path &output)
{
    const auto source = fs::canonical(directory);
    const auto absoluteOutput = fs::absolute(output).lexically_normal();
    const auto relativeOutput = absoluteOutput.lexically_relative(source);

    Require(!fs::exists(output) && !fs::is_symlink(fs::symlink_status(output)), ErrorCode::invalidInput,
            "Refusing to overwrite an archive");
    Require(relativeOutput.empty() || *relativeOutput.begin() == "..", ErrorCode::invalidInput,
            "Archive output must be outside its source directory");

    OpenArchive(output);
    const auto files = CollectPackageFiles(source);

    for (const auto &path : files)
        AppendFile(path, source);

    Require(archive_write_close(writer.get()) == ARCHIVE_OK, ErrorCode::io, "Cannot finish archive");
}
} // namespace baz::detail
