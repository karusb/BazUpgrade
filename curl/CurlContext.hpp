#ifndef _BAZ_CURLCONTEXT_HPP
#define _BAZ_CURLCONTEXT_HPP

#include <curl/curl.h>

class CurlGlobalContext
{
public:
    CurlGlobalContext();
    ~CurlGlobalContext();
};

class CurlContext
{
public:
    CurlContext();
    ~CurlContext();

    CURL* Get();
protected:
    CURL* curl;
};
#endif
