#ifndef _BAZ_RESPONSEPARSERMOCK_HPP
#define _BAZ_RESPONSEPARSERMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/ResponseParser.hpp"

class ResponseParserMock
    : public ResponseParser
{
public:

    MOCK_CONST_METHOD2(ParseReleases, void(const std::string& response, std::vector<Release>& releases));
};
#endif
