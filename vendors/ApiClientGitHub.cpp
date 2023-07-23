#include "ApiClientGitHub.hpp"
#include "util/FILEWrapper.hpp"

ApiClientGitHub::ApiClientGitHub(HTTPClient& client, const std::string& user, const std::string& repo)
    : client(client)
    , user(user)
    , repo(repo)
{}

bool ApiClientGitHub::GetLatestRelease(std::string& response)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/latest";
    return client.Get(url, response) && response.find("\"message\":\"Not Found\"") == std::string::npos;
}

bool ApiClientGitHub::GetReleases(std::string& response)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases";
    return client.Get(url, response) && response.find("\"message\":\"Not Found\"") == std::string::npos;
}

bool ApiClientGitHub::GetRelease(std::string& response, const std::string releaseId)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/" + releaseId;
    return client.Get(url, response) && response.find("\"message\":\"Not Found\"") == std::string::npos;
}

bool ApiClientGitHub::DownloadAsset(const Asset &asset)
{
    FILEWrapper file(asset.filename.c_str(), "wb");
    return file.Error() == 0 && client.Download(asset.url, file.Get());
}
