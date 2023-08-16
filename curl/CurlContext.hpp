#ifndef _BAZ_CURLCONTEXT_HPP
#define _BAZ_CURLCONTEXT_HPP

#include <curl/curl.h>

class CurlGlobalContext
{
public:
    CurlGlobalContext();
    virtual ~CurlGlobalContext();
};

class CurlContext
{
public:
    CurlContext();
    virtual ~CurlContext();

    CURL* GetContext();
private:
    CURL* curl;
};
#endif
