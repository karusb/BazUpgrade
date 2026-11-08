#pragma once

#include "interfaces/Error.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>

namespace baz
{
using Json = nlohmann::json;
namespace fs = std::filesystem;

Json ParseJson(const std::string &bytes);
std::string CanonicalJson(const Json &value);
std::string ReadFile(const fs::path &path, std::uint64_t limit = 4 * 1024 * 1024);
Json ReadJson(const fs::path &path);
void FlushFile(const fs::path &path);
void FlushDirectory(const fs::path &path);
void AtomicWrite(const fs::path &path, const std::string &bytes);
void WriteJson(const fs::path &path, const Json &value);
fs::path SafeRelative(const std::string &value);
std::string Component(const std::string &value);
std::string UtcNow();
std::int64_t ParseUtc(const std::string &value);
} // namespace baz
