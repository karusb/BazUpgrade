#ifndef _BAZ_APICLIENT_HPP
#define _BAZ_APICLIENT_HPP
#include <string>
#include "interfaces/Asset.hpp"

class ApiClient
{
public:
    ApiClient() = default;
    ~ApiClient() = default;
    ApiClient(const ApiClient& other) = delete;
    ApiClient& operator=(const ApiClient& other) = delete;

    virtual bool GetLatestRelease(std::string& response) = 0;
    virtual bool GetReleases(std::string& response) = 0;
    virtual bool GetRelease(std::string& response, const std::string releaseId) = 0;
    virtual bool DownloadAsset(const Asset& asset) = 0;
};
#endif
