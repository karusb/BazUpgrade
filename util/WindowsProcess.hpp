#pragma once
#include "util/Json.hpp"
#include <vector>
namespace baz
{
#ifdef _WIN32
// Fixed executable and argument arrays, bounded inherited handles, retained exit ownership.
int RunWindowsProcess(const std::wstring &executable, const std::vector<std::wstring> &arguments,
                      const fs::path &logPath);
#endif
} // namespace baz
