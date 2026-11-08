#include "controllers/NativeInstaller.hpp"
#include "util/WindowsProcess.hpp"
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
namespace baz
{
namespace
{
#ifdef _WIN32
std::wstring SystemExecutable(const std::wstring &name)
{
    wchar_t directory[MAX_PATH];
    Require(GetSystemDirectoryW(directory, MAX_PATH) != 0, ErrorCode::io, "Cannot resolve system executable");
    return (fs::path(directory) / name).wstring();
}
std::wstring Literal(const std::wstring &value)
{
    std::wstring result = L"'";
    for (wchar_t c : value)
    {
        result += c;
        if (c == static_cast<wchar_t>(39))
            result += c;
    }
    return result + L"'";
}
std::wstring EncodedCommand(const std::wstring &command)
{
    static constexpr wchar_t alphabet[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string bytes;
    for (wchar_t c : command)
    {
        bytes += static_cast<char>(c & 255);
        bytes += static_cast<char>((c >> 8) & 255);
    }
    std::wstring output;
    for (std::size_t i = 0; i < bytes.size(); i += 3)
    {
        unsigned value = static_cast<unsigned char>(bytes[i]) << 16;
        if (i + 1 < bytes.size())
            value |= static_cast<unsigned char>(bytes[i + 1]) << 8;
        if (i + 2 < bytes.size())
            value |= static_cast<unsigned char>(bytes[i + 2]);
        output += alphabet[(value >> 18) & 63];
        output += alphabet[(value >> 12) & 63];
        output += i + 1 < bytes.size() ? alphabet[(value >> 6) & 63] : L'=';
        output += i + 2 < bytes.size() ? alphabet[value & 63] : L'=';
    }
    return output;
}
#else
int Execute(const std::string &executable, const std::vector<std::string> &arguments, const fs::path &logPath)
{
    Require(!fs::is_symlink(fs::symlink_status(logPath)), ErrorCode::io, "Unsafe installer log path");
    auto openStream = [](const char *path, int flags, mode_t mode)
    {
        int descriptor = open(path, flags, mode);
        if (descriptor >= 0 && descriptor < 3)
        {
            int duplicate = fcntl(descriptor, F_DUPFD_CLOEXEC, 3);
            close(descriptor);
            descriptor = duplicate;
        }
        return descriptor;
    };
    int log = openStream(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);
    int input = openStream("/dev/null", O_RDONLY | O_CLOEXEC, 0);
    if (log < 0 || input < 0)
    {
        if (log >= 0)
            close(log);
        if (input >= 0)
            close(input);
        throw Error(ErrorCode::io, "Cannot open installer streams");
    }
    std::vector<char *> argv{const_cast<char *>(executable.c_str())};
    for (const auto &argument : arguments)
        argv.push_back(const_cast<char *>(argument.c_str()));
    argv.push_back(nullptr);
    auto process = fork();
    if (process == 0)
    {
        if (dup2(input, STDIN_FILENO) < 0 || dup2(log, STDOUT_FILENO) < 0 || dup2(log, STDERR_FILENO) < 0)
            _exit(126);
        close(input);
        close(log);
        execv(executable.c_str(), argv.data());
        _exit(127);
    }
    close(log);
    close(input);
    Require(process >= 0, ErrorCode::io, "Native installer could not be started");
    int status = 0;
    while (waitpid(process, &status, 0) < 0)
        if (errno != EINTR)
            throw Error(ErrorCode::unknownOutcome, "Native installer outcome is unknown");
    Require(WIFEXITED(status), ErrorCode::unknownOutcome, "Native installer was interrupted");
    return WEXITSTATUS(status);
}

#endif
} // namespace
bool NativeInstaller::Supports(const std::string &kind)
{
#ifdef _WIN32
    return kind == "msix" || kind == "msi" || kind == "nsis";
#elif defined(__APPLE__)
    return kind == "pkg";
#else
    return kind == "deb" || kind == "rpm";
#endif
}
InstallResult NativeInstaller::Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                                     const Cancellation &cancel)
{
    auto kind = target.custom.at("packageType").get<std::string>();
    Require(Supports(kind), ErrorCode::unsupported, "Installer is unavailable on this platform");
    cancel.Check();
    int code = -1;
    auto log = root / "native-install.log";
#ifdef _WIN32
    if (kind == "msi")
        code = RunWindowsProcess(SystemExecutable(L"msiexec.exe"),
                                 {L"/i", fs::absolute(artifact).wstring(), L"/qn", L"/norestart", L"/L*v",
                                  (root / "msi-install.log").wstring()},
                                 log);
    else if (kind == "nsis")
        code = RunWindowsProcess(fs::absolute(artifact).wstring(), {L"/S"}, log);
    else
    {
        auto command = L"$ErrorActionPreference='Stop'; Add-AppxPackage -Path " +
                       Literal(fs::absolute(artifact).wstring()) + L" -ErrorAction Stop";
        code = RunWindowsProcess(
            SystemExecutable(L"WindowsPowerShell/v1.0/powershell.exe"),
            {L"-NoProfile", L"-NonInteractive", L"-EncodedCommand", EncodedCommand(command)}, log);
    }
#elif defined(__APPLE__)
    code = Execute("/usr/sbin/installer", {"-pkg", fs::absolute(artifact).string(), "-target", "/"}, log);
#else
    if (kind == "deb")
        code = Execute("/usr/bin/dpkg", {"--install", fs::absolute(artifact).string()}, log);
    else
        code = Execute("/usr/bin/rpm", {"--upgrade", fs::absolute(artifact).string()}, log);
#endif
    Require(code == 0 || code == 3010, ErrorCode::unknownOutcome,
            "Installer exited with code " + std::to_string(code) +
                "; inspect native state before reconciling");
    return {code == 3010, ""};
}
void WaitForProcess(std::uint64_t pid, const Cancellation &cancel)
{
    Require(pid > 0 && pid <= 0x7fffffff, ErrorCode::invalidInput, "Invalid process id");
#ifdef _WIN32
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid));
    if (!process)
    {
        Require(GetLastError() == ERROR_INVALID_PARAMETER, ErrorCode::io, "Cannot observe process exit");
        return;
    }
    try
    {
        for (unsigned i = 0; i < 600; ++i)
        {
            cancel.Check();
            auto result = WaitForSingleObject(process, 100);
            if (result == WAIT_OBJECT_0)
            {
                CloseHandle(process);
                return;
            }
            Require(result == WAIT_TIMEOUT, ErrorCode::io, "Cannot observe process exit");
        }
    }
    catch (...)
    {
        CloseHandle(process);
        throw;
    }
    CloseHandle(process);
#else
    for (unsigned i = 0; i < 600; ++i)
    {
        cancel.Check();
        if (kill(static_cast<pid_t>(pid), 0) != 0)
        {
            Require(errno == ESRCH, ErrorCode::io, "Cannot observe process exit");
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
#endif
    throw Error(ErrorCode::busy, "Application did not exit within 60 seconds");
}
} // namespace baz
