#include "JsonHelper.hpp"

namespace JsonHelper
{
    std::string_view FromToken(std::string_view key, std::string_view value, std::size_t offset)
    {
        return value.substr(value.find(key) + offset);
    }

    std::string_view ToToken(std::string_view key, std::string_view value, std::size_t offset)
    {
        return value.substr(0, value.find(key) + offset);
    }

    std::string_view StringValue(std::string_view value) 
    { 
        return ToToken(",", FromToken(":", value, 2), -1);
    }

    std::string_view Value(std::string_view value) 
    {
        return ToToken(",", FromToken(":", value, 1));
    }
}
