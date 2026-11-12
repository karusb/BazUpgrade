#pragma once
#include "util/Json.hpp"
#include <algorithm>

namespace baz
{
inline std::uint64_t ProcessId(const std::string &value)
{
    Require(!value.empty() && value.size() <= 10 &&
                std::all_of(value.begin(), value.end(), [](char c) { return c >= '0' && c <= '9'; }),
            ErrorCode::invalidInput, "Invalid process id");
    auto result = std::stoull(value);
    Require(result <= 2147483647, ErrorCode::invalidInput, "Process id out of range");
    return result;
}
} // namespace baz
