#ifndef _BAZ_JSONHELPER_HPP
#define _BAZ_JSONHELPER_HPP
#include <string>

namespace JsonHelper
{
    std::string_view FromToken(std::string_view key, std::string_view value, std::size_t offset = 0);
    std::string_view ToToken(std::string_view key, std::string_view value, std::size_t offset = 0);
    std::string_view StringValue(std::string_view value);
    std::string_view Value(std::string_view value);
}
#endif

