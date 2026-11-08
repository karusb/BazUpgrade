#pragma once

#include "util/Json.hpp"

namespace baz
{
class FileLock
{
  public:
    explicit FileLock(const fs::path &path);
    ~FileLock();
    FileLock(const FileLock &) = delete;
    FileLock &operator=(const FileLock &) = delete;

  private:
#ifdef _WIN32
    void *handle = nullptr;
#else
    int handle = -1;
#endif
};
} // namespace baz
