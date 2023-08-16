#include <algorithm>
#include "DownloaderSelective.hpp"

DownloaderSelective::DownloaderSelective(ApiClient& apiClient, ResponseParser& parser, AssetSelector& assetSelector, ReleaseSelector& releaseSelector, const std::string& currentVersion, const std::string& releaseIdentifier, const bool& alphaReleases, const bool& downloadTarball, const bool& downloadZipball)
    : apiClient(apiClient)
    , parser(parser)
    , assetSelector(assetSelector)
    , releaseSelector(releaseSelector)
    , currentVersion(currentVersion)
    , releaseIdentifier(releaseIdentifier)
    , alphaReleases(alphaReleases)
    , downloadTarball(downloadTarball)
    , downloadZipball(downloadZipball)
{}

bool DownloaderSelective::Download()
{
    if (!PopulateInformation())
        return false;

    if (latestRelease)
        return SelectivelyDownloadReleaseAssets();

    return true;
}

const std::string& DownloaderSelective::LatestVersion() const
{
    return latestRelease->version;
}

const std::vector<Asset>& DownloaderSelective::LastReleaseAssets() const
{
    return latestRelease->assets;
}

std::optional<Asset> DownloaderSelective::SelectedAsset() const
{
    return selectedAsset;
}

void DownloaderSelective::ParseReleases()
{
    parser.ParseReleases(response, releases);
    latestRelease = releaseSelector.SelectEligibleRelease(currentVersion, releases);
    selectedAsset = assetSelector.SelectAsset(releaseIdentifier, latestRelease->assets);
}

bool DownloaderSelective::DownloadLatestReleaseAssets()
{
    bool result = true;
    for (const auto& asset : latestRelease->assets)
        result = result && apiClient.DownloadAsset(asset);
    return result;
}

bool DownloaderSelective::DownloadSelectedReleaseAsset()
{
    if (!selectedAsset)
        return false;
    return apiClient.DownloadAsset(*selectedAsset);
}

bool DownloaderSelective::SelectivelyDownloadReleaseAssets()
{
    bool result = true;
    if (!latestRelease->assets.empty())
    {
        if (releaseIdentifier == "")
            result = result && DownloadLatestReleaseAssets();
        else
            result = result && DownloadSelectedReleaseAsset();
    }

    if (downloadTarball)
        result = result && apiClient.DownloadAsset(latestRelease->tarball);
    if (downloadZipball)
        result = result && apiClient.DownloadAsset(latestRelease->zipball);

    return result;
}

bool DownloaderSelective::PopulateInformation()
{
    if (alphaReleases)
    {
        if (!apiClient.GetReleases(response))
            return false;
    }
    else if (!apiClient.GetLatestRelease(response))       
        return false;

    ParseReleases();

    return true;
}
