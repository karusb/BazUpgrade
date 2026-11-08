#include "storage/FileLock.hpp"
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace baz
{
FileLock::FileLock(const fs::path &path)
{
    fs::create_directories(path.parent_path());
    Require(!fs::is_symlink(fs::symlink_status(path)), ErrorCode::io, "Unsafe lock path");
#ifdef _WIN32
    handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
    Require(handle != INVALID_HANDLE_VALUE, ErrorCode::busy, "Another operation owns this resource");
#else
    handle = open(path.c_str(), O_RDWR | O_CREAT | O_NOFOLLOW, 0600);
    Require(handle >= 0, ErrorCode::io, "Cannot open operation lock");
    if (flock(handle, LOCK_EX | LOCK_NB) != 0)
    {
        close(handle);
        handle = -1;
        throw Error(ErrorCode::busy, "Another operation owns this resource");
    }
#endif
}

FileLock::~FileLock()
{
#ifdef _WIN32
    if (handle && handle != INVALID_HANDLE_VALUE)
        CloseHandle(handle);
#else
    if (handle >= 0)
    {
        flock(handle, LOCK_UN);
        close(handle);
    }
#endif
}
} // namespace baz
