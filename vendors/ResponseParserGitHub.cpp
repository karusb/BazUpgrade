#include "ResponseParserGitHub.hpp"
#include "util/JsonHelper.hpp"

std::string_view ResponseParserGitHub::TarballUrl(std::string_view response) const
{ 
    return JsonHelper::StringValue(JsonHelper::FromToken("\"tarball_url\":", response));
}

std::string_view ResponseParserGitHub::ZipballUrl(std::string_view response) const
{
    return JsonHelper::StringValue(JsonHelper::FromToken("\"zipball_url\":", response));
}

std::string_view ResponseParserGitHub::FilenameFromUrl(std::string_view url) const
{
    return url.substr(url.find_last_of("/") + 1);
}

std::string_view ResponseParserGitHub::TagName(std::string_view response) const
{
    return JsonHelper::StringValue(JsonHelper::FromToken("\"tag_name\":", response));
}

std::string_view ResponseParserGitHub::Name(std::string_view response) const
{
    return JsonHelper::StringValue(JsonHelper::FromToken("\"name\":", response));
}

bool ResponseParserGitHub::Draft(std::string_view response) const
{
    return JsonHelper::Value(JsonHelper::FromToken("\"draft\":", response)) == "true";
}

bool ResponseParserGitHub::PreRelease(std::string_view response) const
{
    return JsonHelper::Value(JsonHelper::FromToken("\"prerelease\":", response)) == "true";
}

std::vector<std::string_view> ResponseParserGitHub::DivideReleases(std::string_view response) const
{
    std::vector<std::string_view> parsed;

    for (auto releaseStart = response.find("{\"url\":"); releaseStart != response.npos; releaseStart = response.find("},{\"url\":", response.find("zipball_url", releaseStart + 8)))
    {
        auto endofRelease = response.find("zipball_url", releaseStart + 8);
        auto nextUrl = response.find("},{\"url\":", endofRelease);
            if (nextUrl == response.npos)
                nextUrl = response.find("}", endofRelease);
            if (nextUrl == response.npos)
                nextUrl = response.find("}]", endofRelease);
        parsed.push_back(response.substr(releaseStart, nextUrl - releaseStart));
    }

    return parsed;
}

void ResponseParserGitHub::ParseReleases(const std::string &response, std::vector<Release>& releases) const
{
    std::string zipExtension = ".zip";
    std::string tarExtension = ".tar.gz";
    auto jsonReleases = DivideReleases(response);
    for (auto& jsonRelease : jsonReleases)
    {
        std::vector<Asset> assets;
        for (auto& asset : AssetUrls(jsonRelease))
            assets.push_back({std::string(asset), std::string(FilenameFromUrl(asset))});
            
        releases.push_back(
        {
            std::string(TagName(jsonRelease)), std::string(Name(jsonRelease)), assets,
            { std::string(TarballUrl(jsonRelease)), std::string(FilenameFromUrl(TarballUrl(jsonRelease))) + tarExtension},
            { std::string(ZipballUrl(jsonRelease)), std::string(FilenameFromUrl(ZipballUrl(jsonRelease))) + zipExtension},
            PreRelease(jsonRelease), Draft(jsonRelease)
        });
    }

}

std::vector<std::string_view> ResponseParserGitHub::AssetUrls(std::string_view response) const
{
    std::vector<std::string_view> parsed;
    std::string_view assets;
    std::string_view asset;
    
    assets = JsonHelper::FromToken("\"assets\":[", response);
    while (assets.find("\"browser_download_url\":") != std::string::npos)
    {
        assets = JsonHelper::FromToken("\"browser_download_url\":", assets);    
        assets = JsonHelper::FromToken(":\"", assets, 2);
        asset = JsonHelper::ToToken("\"", assets);
        parsed.push_back(asset);
    }

    return parsed;
}
