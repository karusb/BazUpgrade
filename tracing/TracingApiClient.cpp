#include "TracingApiClient.hpp"

TracingApiClient::TracingApiClient(ApiClient& client, std::ostream& stream)
    : client(client)
    , stream(stream)
{}

bool TracingApiClient::GetLatestRelease(std::string& response)
{
    stream << "Requesting latest release... ";
    auto result = client.GetLatestRelease(response);
    stream << result << std::endl;
    return result; 
}

bool TracingApiClient::GetReleases(std::string& response)
{
    stream << "Requesting all releases... ";
    auto result = client.GetReleases(response);
    stream << result << std::endl;
    return result; 
}

bool TracingApiClient::GetRelease(std::string& response, const std::string releaseId)
{
    stream << "Requesting release " << releaseId << " ... ";
    auto result = client.GetRelease(response, releaseId);
    stream << result << std::endl;
    return result; 
}

bool TracingApiClient::DownloadAsset(const Asset &asset)
{
    stream << "Downloading asset " << asset.filename << " ... ";
    auto result = client.DownloadAsset(asset);
    stream << result << std::endl;
    return result; 
}
