#ifndef _BAZ_RESPONSEPARSER_HPP
#define _BAZ_RESPONSEPARSER_HPP
#include <vector>
#include <optional>
#include <string>
#include "Release.hpp"

class ResponseParser
{
public:

    ResponseParser() = default;
    ~ResponseParser() = default;
    ResponseParser(const ResponseParser& other) = delete;
    ResponseParser& operator=(const ResponseParser& other) = delete;

    virtual void ParseReleases(const std::string& response, std::vector<Release>& releases) const = 0;
};
#endif
