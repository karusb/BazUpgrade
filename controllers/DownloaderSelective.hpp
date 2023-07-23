#ifndef _BAZ_DOWNLOADERSELECTIVE_HPP
#define _BAZ_DOWNLOADERSELECTIVE_HPP
#include "interfaces/ApiClient.hpp"
#include "interfaces/AssetSelector.hpp"
#include "interfaces/Downloader.hpp"
#include "interfaces/HttpClient.hpp"
#include "interfaces/ResponseParser.hpp"
#include "interfaces/ReleaseSelector.hpp"

class DownloaderSelective
    : public Downloader
{
public:
    DownloaderSelective(ApiClient& apiClient, ResponseParser& parser, AssetSelector& assetSelector, ReleaseSelector& releaseSelector, const std::string& currentVersion, const std::string& releaseIdentifier, const bool& alphaReleases, const bool& downloadTarball, const bool& downloadZipball);

    bool Download() override;
    const std::string& LatestVersion() const override;
    const std::vector<Asset>& LastReleaseAssets() const override;
    std::optional<Asset> SelectedAsset() const override;

protected:
    void ParseReleases();
    bool DownloadLatestReleaseAssets();
    bool DownloadSelectedReleaseAsset();
    bool SelectivelyDownloadReleaseAssets();
    bool PopulateInformation();
    
private:
    ApiClient& apiClient;
    ResponseParser& parser;
    AssetSelector& assetSelector;
    ReleaseSelector& releaseSelector;
    const std::string& currentVersion;
    const std::string& releaseIdentifier;
    const bool& alphaReleases;
    const bool& downloadTarball;
    const bool& downloadZipball;

    std::vector<Release> releases;
    std::optional<Release> latestRelease;
    std::optional<Asset> selectedAsset;
    std::string response;
};
#endif
