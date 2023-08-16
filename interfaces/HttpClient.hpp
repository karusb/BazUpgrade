#ifndef _BAZ_HTTPCLIENT_HPP
#define _BAZ_HTTPCLIENT_HPP
#include <string>

class HTTPClient
{
public:
    HTTPClient() = default;
    virtual ~HTTPClient() = default;
    HTTPClient(const HTTPClient& other) = delete;
    HTTPClient& operator=(const HTTPClient& other) = delete;

    virtual bool Get(const std::string& url, std::string& response) = 0;
    virtual bool Post(const std::string& url, std::string& fields, std::string& response) = 0;
    virtual bool Download(const std::string& url, FILE* data) = 0;
};
#endif