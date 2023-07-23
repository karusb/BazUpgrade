#ifndef _BAZ_APICLIENTGITHUB_HPP
#define _BAZ_APICLIENTGITHUB_HPP
#include "interfaces/HttpClient.hpp"
#include "interfaces/ApiClient.hpp"

class ApiClientGitHub
    : public ApiClient
{
public:
    ApiClientGitHub(HTTPClient& client, const std::string& user, const std::string& repo);

    bool GetLatestRelease(std::string& response) override;
    bool GetReleases(std::string& response) override;
    bool GetRelease(std::string& response, const std::string releaseId) override;
    bool DownloadAsset(const Asset& asset) override;
    
private:
    HTTPClient& client;
    const std::string& user;
    const std::string& repo;
};
#endif
