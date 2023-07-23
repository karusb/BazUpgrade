#ifndef _BAZ_TRACINGAPICLIENT_HPP
#define _BAZ_TRACINGAPICLIENT_HPP
#include "interfaces/ApiClient.hpp"
#include <iostream>

class TracingApiClient
    : public ApiClient
{
public:
    TracingApiClient(ApiClient& client, std::ostream& stream);

    bool GetLatestRelease(std::string& response) override;
    bool GetReleases(std::string& response) override;
    bool GetRelease(std::string& response, const std::string releaseId) override;
    bool DownloadAsset(const Asset& asset) override;
    
private:
    ApiClient& client;
    std::ostream& stream;
};
#endif
