#pragma once

#include "util/Json.hpp"
#include <archive.h>
#include <memory>
#include <vector>

namespace baz::detail
{
class PortableArchiveWriter final
{
  public:
    PortableArchiveWriter();
    void WriteDirectory(const fs::path &directory, const fs::path &output);

  private:
    void OpenArchive(const fs::path &output);
    static std::vector<fs::path> CollectPackageFiles(const fs::path &source);
    void AppendFile(const fs::path &path, const fs::path &source);
    void CopyFileContents(const fs::path &path);

    std::unique_ptr<archive, decltype(&archive_write_free)> writer;
};
} // namespace baz::detail
