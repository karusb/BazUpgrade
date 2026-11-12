#include "util/Json.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace baz
{
void Require(bool condition, ErrorCode code, const std::string &message)
{
    if (!condition)
        throw Error(code, message);
}

Json ParseJson(const std::string &bytes)
{
    // Reject duplicate members before a DOM parser could overwrite their values.
    std::vector<std::set<std::string>> members;

    auto callback = [&](int depth, Json::parse_event_t event, Json &value)
    {
        Require(depth <= 128, ErrorCode::trust, "JSON nesting limit exceeded");
        if (event == Json::parse_event_t::object_start)
            members.emplace_back();
        if (event == Json::parse_event_t::key)
            Require(members.back().insert(value.get<std::string>()).second, ErrorCode::trust,
                    "Duplicate JSON member");
        if (event == Json::parse_event_t::object_end)
            members.pop_back();
        return true;
    };

    try
    {
        return Json::parse(bytes, callback);
    }
    catch (const Json::exception &e)
    {
        throw Error(ErrorCode::invalidInput, std::string("Invalid JSON: ") + e.what());
    }
}

std::string CanonicalJson(const Json &value)
{
    // TUF/OLPC canonical JSON is deliberately not RFC 8785 or json::dump().
    if (value.is_string())
    {
        std::string out = "\"";
        for (char c : value.get<std::string>())
        {
            if (c == '\\' || c == '"')
                out += '\\';
            out += c;
        }
        return out + '"';
    }
    if (value.is_number_integer() || value.is_number_unsigned())
        return value.dump();
    Require(!value.is_number_float(), ErrorCode::trust, "TUF canonical JSON forbids floating point values");
    if (value.is_array() || value.is_object())
    {
        std::string out = value.is_array() ? "[" : "{";
        bool first = true;
        for (auto it = value.begin(); it != value.end(); ++it)
        {
            if (!first)
                out += ',';
            first = false;
            if (value.is_object())
                out += CanonicalJson(it.key()) + ':';
            out += CanonicalJson(it.value());
        }
        return out + (value.is_array() ? ']' : '}');
    }
    return value.dump();
}

std::string ReadFile(const fs::path &path, std::uint64_t limit)
{
    std::ifstream file(path, std::ios::binary);

    Require(file.good(), ErrorCode::io, "Cannot open file: " + path.filename().u8string());

    std::string out;
    char buffer[16384];

    while (file.read(buffer, sizeof(buffer)) || file.gcount())
    {
        Require(out.size() + static_cast<std::uint64_t>(file.gcount()) <= limit, ErrorCode::io,
                "File exceeds size limit");
        out.append(buffer, static_cast<std::size_t>(file.gcount()));
    }
    Require(file.eof(), ErrorCode::io, "File read failed");

    return out;
}

Json ReadJson(const fs::path &path)
{
    return ParseJson(ReadFile(path));
}

void FlushFile(const fs::path &path)
{
#ifdef _WIN32
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    Require(file != INVALID_HANDLE_VALUE, ErrorCode::io, "Cannot open file for persistence");
    bool ok = FlushFileBuffers(file) != 0;
    CloseHandle(file);
#else
    int file = open(path.c_str(), O_RDONLY | O_NOFOLLOW);
    Require(file >= 0, ErrorCode::io, "Cannot open file for persistence");
    bool ok = fsync(file) == 0;
    close(file);
#endif
    Require(ok, ErrorCode::io, "Cannot persist file contents");
}

void FlushDirectory(const fs::path &path)
{
#ifndef _WIN32
    auto directoryPath = path.empty() ? fs::path(".") : path;
    int directory = open(directoryPath.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    Require(directory >= 0, ErrorCode::io, "Cannot open directory for persistence");
    bool ok = fsync(directory) == 0;
    close(directory);
    Require(ok, ErrorCode::io, "Cannot persist directory entries");
#else
    // Windows state replacement uses MoveFileExW with MOVEFILE_WRITE_THROUGH.
    (void)path;
#endif
}

void AtomicWrite(const fs::path &path, const std::string &bytes)
{
    if (!path.parent_path().empty())
        fs::create_directories(path.parent_path());

    auto temporary = path;
    temporary += ".next";

    Require(!fs::is_symlink(fs::symlink_status(path)) && !fs::exists(temporary), ErrorCode::io,
            "Unsafe or interrupted state write; inspect .next file");
#ifdef _WIN32
    HANDLE file =
        CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    Require(file != INVALID_HANDLE_VALUE, ErrorCode::io, "Cannot create state file");
    DWORD written = 0;
    bool ok = bytes.size() <= MAXDWORD &&
              WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
              written == bytes.size() && FlushFileBuffers(file);
    CloseHandle(file);
    Require(ok, ErrorCode::io, "Cannot persist state file");
    Require(MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH),
            ErrorCode::io, "Cannot commit state file");
#else
    int file = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    Require(file >= 0, ErrorCode::io, "Cannot create state file");
    std::size_t offset = 0;
    while (offset < bytes.size())
    {
        auto written = write(file, bytes.data() + offset, bytes.size() - offset);
        if (written <= 0)
        {
            close(file);
            throw Error(ErrorCode::io, "Cannot persist state file");
        }
        offset += static_cast<std::size_t>(written);
    }
    bool ok = fsync(file) == 0;
    close(file);
    Require(ok, ErrorCode::io, "Cannot flush state file");
    fs::rename(temporary, path);
    FlushDirectory(path.parent_path());
#endif
}

void WriteJson(const fs::path &path, const Json &value)
{
    AtomicWrite(path, value.dump(2) + '\n');
}

fs::path SafeRelative(const std::string &value)
{
    Require(!value.empty() && value.size() <= 1024 && value.front() != '/' && value.back() != '/',
            ErrorCode::invalidInput, "Invalid relative path");
    Require(value.find_first_of("\\:%?#") == std::string::npos && value.find('\0') == std::string::npos,
            ErrorCode::invalidInput, "Unsafe relative path");
    fs::path result;
    std::istringstream input(value);
    std::string part;
    while (std::getline(input, part, '/'))
    {
        Require(!part.empty() && part != "." && part != ".." && part.back() != '.' && part.back() != ' ',
                ErrorCode::invalidInput, "Unsafe path component");
        Require(std::all_of(part.begin(), part.end(), [](unsigned char c) { return c >= 32 && c != 127; }),
                ErrorCode::invalidInput, "Control character in path");
        std::string base = part.substr(0, part.find('.'));
        std::transform(base.begin(), base.end(), base.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        Require(base != "CONIN$" && base != "CONOUT$" && base != "CLOCK$" && base != "CON" && base != "PRN" &&
                    base != "AUX" && base != "NUL" &&
                    !(base.size() == 4 && (base.substr(0, 3) == "COM" || base.substr(0, 3) == "LPT") &&
                      base[3] >= '1' && base[3] <= '9'),
                ErrorCode::invalidInput, "Reserved path component");
        result /= fs::u8path(part);
    }
    return result;
}

std::string Component(const std::string &value)
{
    Require(!value.empty() && value.size() <= 100 && value != "." && value != ".." &&
                std::all_of(value.begin(), value.end(), [](unsigned char c)
                            { return std::isalnum(c) || c == '-' || c == '_' || c == '.'; }),
            ErrorCode::invalidInput, "Invalid identifier");
    SafeRelative(value);
    return value;
}

std::string UtcNow()
{
    auto now = std::time(nullptr);
    std::tm utc{};
#ifdef _WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

std::int64_t ParseUtc(const std::string &value)
{
    Require(value.size() == 20 && value.back() == 'Z', ErrorCode::trust, "Expected UTC timestamp");
    std::tm utc{};
    std::istringstream input(value);
    input >> std::get_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    Require(!input.fail(), ErrorCode::trust, "Invalid UTC timestamp");
#ifdef _WIN32
    auto result = _mkgmtime(&utc);
#else
    auto result = timegm(&utc);
#endif
    Require(result >= 0, ErrorCode::trust, "Invalid UTC timestamp");
    std::ostringstream normalized;
    normalized << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    Require(normalized.str() == value, ErrorCode::trust, "Invalid calendar date");
    return result;
}
} // namespace baz
