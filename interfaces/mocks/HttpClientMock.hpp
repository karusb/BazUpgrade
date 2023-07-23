#ifndef _BAZ_HTTPCLIENTMOCK_HPP
#define _BAZ_HTTPCLIENTMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/HttpClient.hpp"

class HTTPClientMock
    : public HTTPClient
{
public:
    MOCK_METHOD2(Get, bool(const std::string& url, std::string& response));
    MOCK_METHOD3(Post, bool(const std::string& url, std::string& fields, std::string& response));
    MOCK_METHOD2(Download, bool(const std::string& url, FILE* data));
};
#endif