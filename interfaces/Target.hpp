#pragma once
#include "util/Json.hpp"

namespace baz
{
struct Target
{
    std::string path;
    std::uint64_t length;
    Json hashes;
    Json custom;
};
} // namespace baz
