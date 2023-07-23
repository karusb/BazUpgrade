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

    bool Get(const std::string& url, std::string& response) override;
    bool Post(const std::string& url, std::string& fields, std::string& response) override;
    bool Download(const std::string& url, FILE* data) override;

private:
    bool PerformCurlRequest();

    struct curl_slist* headers = NULL;
};
#endif
