#pragma once

#include "interfaces/Transport.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <iosfwd>
#include <memory>
#include <set>

namespace baz::detail
{
class PortableArchiveReader final
{
  public:
    PortableArchiveReader(std::uint64_t maxBytes, std::uint64_t maxEntries, const Cancellation &cancel);
    void ExtractTo(const fs::path &artifact, const fs::path &destination);

  private:
    void OpenArchive(const fs::path &artifact);
    fs::path ReadRelativeEntryPath(archive_entry *entry);
    void ExtractEntry(archive_entry *entry, const fs::path &destination);
    void ExtractFile(archive_entry *entry, const fs::path &output);
    void CopyEntryContents(std::ostream &output);
    static void SetExtractedPermissions(archive_entry *entry, const fs::path &output);

    std::unique_ptr<archive, decltype(&archive_read_free)> input;
    const std::uint64_t maxBytes;
    const std::uint64_t maxEntries;
    const Cancellation &cancel;
    std::uint64_t entryCount = 0;
    std::uint64_t receivedBytes = 0;
    std::set<std::string> paths;
};
} // namespace baz::detail
