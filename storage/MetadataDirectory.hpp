#pragma once
#include "util/Json.hpp"
#include <memory>
namespace baz
{
// An anchored metadata directory. No metadata I/O follows repository symlinks.
class MetadataDirectory
{
  public:
    explicit MetadataDirectory(const fs::path &repository);
    ~MetadataDirectory();
    Json Read(const std::string &name) const;
    void Write(const std::string &name, const Json &value) const;
    MetadataDirectory(const MetadataDirectory &) = delete;
    MetadataDirectory &operator=(const MetadataDirectory &) = delete;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace baz
