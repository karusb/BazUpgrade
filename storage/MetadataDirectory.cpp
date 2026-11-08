#include "storage/MetadataDirectory.hpp"
#include "controllers/PortableInstaller.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include <cstring>
#include <openssl/rand.h>
#ifdef _WIN32
#include <windows.h>
#include <winternl.h>
extern "C" NTSYSAPI NTSTATUS NTAPI NtSetInformationFile(HANDLE, PIO_STATUS_BLOCK, PVOID, ULONG,
                                                        FILE_INFORMATION_CLASS);
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace baz
{
namespace
{
constexpr std::size_t MetadataLimit = 4 * 1024 * 1024;
#ifdef _WIN32
struct Handle
{
    HANDLE value = INVALID_HANDLE_VALUE;
    bool discard = false;
    ~Handle()
    {
        if (value != INVALID_HANDLE_VALUE)
        {
            if (discard)
            {
                FILE_DISPOSITION_INFO info{TRUE};
                SetFileInformationByHandle(value, FileDispositionInfo, &info, sizeof(info));
            }
            CloseHandle(value);
        }
    }
};
void LockDirectory(Handle &handle, const fs::path &path)
{
    // Attribute-only access does not participate in Windows sharing checks.
    // Read/list access is required for this handle to deny directory deletion.
    handle.value =
        CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    Require(handle.value != INVALID_HANDLE_VALUE, ErrorCode::io, "Cannot anchor metadata directory");
    FILE_ATTRIBUTE_TAG_INFO info{};
    Require(GetFileInformationByHandleEx(handle.value, FileAttributeTagInfo, &info, sizeof(info)) &&
                (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                !(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT),
            ErrorCode::io, "Unsafe metadata directory");
}
void OpenRelative(Handle &handle, HANDLE directory, const std::string &name, ACCESS_MASK access,
                  ULONG disposition, ULONG options)
{
    auto wide = fs::u8path(name).wstring();
    Require(wide.size() * sizeof(wchar_t) <= 65534, ErrorCode::invalidInput, "Metadata name is too long");
    UNICODE_STRING relative{};
    relative.Length = relative.MaximumLength = static_cast<USHORT>(wide.size() * sizeof(wchar_t));
    relative.Buffer = wide.data();
    OBJECT_ATTRIBUTES attributes{};
    attributes.Length = sizeof(attributes);
    attributes.RootDirectory = directory;
    attributes.ObjectName = &relative;
    attributes.Attributes = OBJ_CASE_INSENSITIVE | 0x1000; // OBJ_DONT_REPARSE
    IO_STATUS_BLOCK status{};
    auto result =
        NtCreateFile(&handle.value, access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attributes, &status,
                     nullptr, FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ | FILE_SHARE_WRITE, disposition,
                     options | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT, nullptr, 0);
    Require(result >= 0, ErrorCode::io, "Cannot open anchored metadata entry");
    FILE_ATTRIBUTE_TAG_INFO info{};
    Require(GetFileInformationByHandleEx(handle.value, FileAttributeTagInfo, &info, sizeof(info)) &&
                !(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT),
            ErrorCode::io, "Unsafe metadata entry");
}
#else
struct Descriptor
{
    int value = -1;
    ~Descriptor()
    {
        if (value >= 0)
            close(value);
    }
};
#endif
} // namespace
struct MetadataDirectory::Impl
{
    fs::path path;
#ifdef _WIN32
    Handle root, metadata;
#else
    Descriptor root, metadata;
#endif
};
MetadataDirectory::MetadataDirectory(const fs::path &repository) : impl(std::make_unique<Impl>())
{
    SecureDirectory(repository);
    impl->path = repository / "metadata";
#ifdef _WIN32
    LockDirectory(impl->root, repository);
    OpenRelative(impl->metadata, impl->root.value, "metadata", GENERIC_READ, FILE_OPEN_IF,
                 FILE_DIRECTORY_FILE);
#else
    impl->root.value = open(repository.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    Require(impl->root.value >= 0, ErrorCode::io, "Cannot anchor repository directory");
    Require(mkdirat(impl->root.value, "metadata", 0755) == 0 || errno == EEXIST, ErrorCode::io,
            "Cannot create metadata directory");
    impl->metadata.value =
        openat(impl->root.value, "metadata", O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    Require(impl->metadata.value >= 0, ErrorCode::io, "Unsafe metadata directory");
#endif
}
MetadataDirectory::~MetadataDirectory() = default;
Json MetadataDirectory::Read(const std::string &name) const
{
    Component(name);
    std::string bytes;
#ifdef _WIN32
    Handle file;
    OpenRelative(file, impl->metadata.value, name, GENERIC_READ, FILE_OPEN, FILE_NON_DIRECTORY_FILE);
    FILE_ATTRIBUTE_TAG_INFO info{};
    Require(GetFileInformationByHandleEx(file.value, FileAttributeTagInfo, &info, sizeof(info)) &&
                !(info.FileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)),
            ErrorCode::io, "Unsafe metadata file");
    char buffer[16384];
    DWORD received = 0;
    while (true)
    {
        Require(::ReadFile(file.value, buffer, sizeof(buffer), &received, nullptr), ErrorCode::io,
                "Metadata read failed");
        if (!received)
            break;
        Require(bytes.size() + received <= MetadataLimit, ErrorCode::io, "Metadata exceeds size limit");
        bytes.append(buffer, received);
    }
#else
    Descriptor file;
    file.value = openat(impl->metadata.value, name.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK);
    Require(file.value >= 0, ErrorCode::io, "Cannot read metadata file");
    struct stat info
    {
    };
    Require(fstat(file.value, &info) == 0 && S_ISREG(info.st_mode), ErrorCode::io, "Unsafe metadata file");
    char buffer[16384];
    while (true)
    {
        auto received = read(file.value, buffer, sizeof(buffer));
        if (received < 0 && errno == EINTR)
            continue;
        Require(received >= 0, ErrorCode::io, "Metadata read failed");
        if (!received)
            break;
        Require(bytes.size() + static_cast<std::size_t>(received) <= MetadataLimit, ErrorCode::io,
                "Metadata exceeds size limit");
        bytes.append(buffer, static_cast<std::size_t>(received));
    }
#endif
    return ParseJson(bytes);
}
void MetadataDirectory::Write(const std::string &name, const Json &value) const
{
    Component(name);
    auto bytes = value.dump(2) + '\n';
    Require(bytes.size() <= MetadataLimit, ErrorCode::invalidInput, "Metadata exceeds size limit");
    unsigned char nonce[16];
    Require(RAND_bytes(nonce, sizeof(nonce)) == 1, ErrorCode::io,
            "Cannot allocate metadata transaction name");
    auto temporary = name + "." + Hex(nonce, sizeof(nonce)) + ".next";
#ifdef _WIN32
    // Open and rename relative to the directory handle. No pathname is resolved
    // through a replaceable directory or junction during this transaction.
    Handle file;
    OpenRelative(file, impl->metadata.value, temporary, GENERIC_WRITE | DELETE, FILE_CREATE,
                 FILE_NON_DIRECTORY_FILE | FILE_WRITE_THROUGH);
    file.discard = true;
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        auto count = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset, 16384));
        DWORD written = 0;
        Require(::WriteFile(file.value, bytes.data() + offset, count, &written, nullptr) && written,
                ErrorCode::io, "Metadata write failed");
        offset += written;
    }
    Require(FlushFileBuffers(file.value), ErrorCode::io, "Cannot flush metadata transaction");
    auto destination = fs::u8path(name).wstring();
    auto filenameBytes = static_cast<DWORD>(destination.size() * sizeof(wchar_t));
    std::vector<BYTE> buffer(sizeof(FILE_RENAME_INFO) + filenameBytes);
    auto rename = reinterpret_cast<FILE_RENAME_INFO *>(buffer.data());
    rename->ReplaceIfExists = TRUE;
    rename->RootDirectory = impl->metadata.value;
    rename->FileNameLength = filenameBytes;
    std::memcpy(rename->FileName, destination.data(), filenameBytes);
    // The Win32 wrapper rejects non-null RootDirectory on supported Windows
    // builds. Use the native rename operation to retain directory-relative I/O.
    IO_STATUS_BLOCK status{};
    constexpr auto renameInformation = static_cast<FILE_INFORMATION_CLASS>(10); // FileRenameInformation
    auto result = NtSetInformationFile(file.value, &status, rename, static_cast<ULONG>(buffer.size()),
                                       renameInformation);
    Require(result >= 0, ErrorCode::io,
            "Cannot persist anchored metadata (NTSTATUS " + std::to_string(result) + ")");
    file.discard = false;
    Require(FlushFileBuffers(file.value), ErrorCode::io, "Cannot flush committed metadata");
#else
    struct Cleanup
    {
        int directory;
        const std::string &name;
        ~Cleanup()
        {
            unlinkat(directory, name.c_str(), 0);
        }
    } cleanup{impl->metadata.value, temporary};
    Descriptor file;
    file.value = openat(impl->metadata.value, temporary.c_str(),
                        O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC, 0644);
    Require(file.value >= 0, ErrorCode::io, "Unsafe or interrupted metadata write");
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        auto written = write(file.value, bytes.data() + offset, bytes.size() - offset);
        if (written < 0 && errno == EINTR)
            continue;
        Require(written > 0, ErrorCode::io, "Metadata write failed");
        offset += static_cast<std::size_t>(written);
    }
    Require(fsync(file.value) == 0 &&
                renameat(impl->metadata.value, temporary.c_str(), impl->metadata.value, name.c_str()) == 0 &&
                fsync(impl->metadata.value) == 0,
            ErrorCode::io, "Cannot persist metadata");
#endif
}
} // namespace baz
