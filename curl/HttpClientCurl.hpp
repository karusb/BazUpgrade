#ifndef _BAZ_HTTPCLIENTCURL_HPP
#define _BAZ_HTTPCLIENTCURL_HPP
#include "CurlContext.hpp"
#include "DataCallbacks.hpp"
#include "interfaces/HttpClient.hpp"

class HTTPClientCurl
    : public HTTPClient
    , public CurlContext
    , public CurlGlobalContext
{
public:
    HTTPClientCurl();

    virtual bool Get(const std::string& url, std::string& response) override;
    virtual bool Post(const std::string& url, std::string& fields, std::string& response) override;
    virtual bool Download(const std::string& url, FILE* data) override;

private:
    bool PerformCurlRequest();

    struct curl_slist* headers = nullptr;
};
#endif
