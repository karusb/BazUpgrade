#include "CurlContext.hpp"

CurlContext::CurlContext()
{
    curl = curl_easy_init();
}

CurlContext::~CurlContext()
{
    curl_easy_cleanup(curl);
}

CURL* CurlContext::GetContext()
{
    return curl;
}

CurlGlobalContext::CurlGlobalContext()
{
    curl_global_init(CURL_GLOBAL_ALL);
}

CurlGlobalContext::~CurlGlobalContext()
{
    curl_global_cleanup();
}
