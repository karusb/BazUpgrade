#include "util/WindowsProcess.hpp"
#include <chrono>
#include <thread>
#include <windows.h>

namespace baz
{
namespace
{
std::wstring QuoteArgument(const std::wstring &input)
{
    // msiexec treats quoted option switches differently from CRT argv parsers.
    // No shell is involved; quote only arguments that need Windows escaping.
    if (!input.empty() && input.find_first_of(L" \t\r\n\v\"") == std::wstring::npos)
        return input;
    std::wstring output = L"\"";
    unsigned slashes = 0;
    for (wchar_t c : input)
    {
        if (c == L'\\')
        {
            ++slashes;
            continue;
        }
        output.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        output += c;
    }
    output.append(slashes * 2, L'\\');
    return output + L'"';
}
} // namespace

int RunWindowsProcess(const std::wstring &executable, const std::vector<std::wstring> &arguments,
                      const fs::path &logPath)
{
    Require(!fs::is_symlink(fs::symlink_status(logPath)), ErrorCode::io, "Unsafe installer log path");

    struct Handle
    {
        HANDLE value;

        ~Handle()
        {
            if (value != INVALID_HANDLE_VALUE && value)
                CloseHandle(value);
        }
    };

    SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle log{CreateFileW(logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr)};
    Handle input{CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    Require(log.value != INVALID_HANDLE_VALUE && input.value != INVALID_HANDLE_VALUE, ErrorCode::io,
            "Cannot open installer streams");
    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<unsigned char> storage(size);
    auto attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    Require(InitializeProcThreadAttributeList(attributes, 1, 0, &size), ErrorCode::io,
            "Cannot prepare installer process");

    struct AttributeGuard
    {
        LPPROC_THREAD_ATTRIBUTE_LIST value;

        ~AttributeGuard()
        {
            DeleteProcThreadAttributeList(value);
        }
    } guard{attributes};

    HANDLE handles[] = {log.value, input.value};
    Require(UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles,
                                      sizeof(handles), nullptr, nullptr),
            ErrorCode::io, "Cannot restrict installer handles");
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = input.value;
    startup.StartupInfo.hStdOutput = startup.StartupInfo.hStdError = log.value;
    startup.lpAttributeList = attributes;
    auto command = QuoteArgument(executable);
    for (const auto &argument : arguments)
        command += L" " + QuoteArgument(argument);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT, nullptr,
                        logPath.parent_path().c_str(), &startup.StartupInfo, &process))
    {
        auto error = GetLastError();
        throw Error(ErrorCode::io,
                    "Windows process could not be started (Win32 error " + std::to_string(error) + ")");
    }
    Handle thread{process.hThread}, child{process.hProcess};
    // A transient observation failure must not release ownership of executing work.
    while (WaitForSingleObject(child.value, INFINITE) != WAIT_OBJECT_0)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    DWORD exitCode = 0;
    Require(GetExitCodeProcess(child.value, &exitCode), ErrorCode::unknownOutcome,
            "Native installer exit status is unknown");
    return static_cast<int>(exitCode);
}
} // namespace baz
