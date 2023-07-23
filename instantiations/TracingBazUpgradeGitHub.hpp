#ifndef _BAZ_TRACINGBAZUPGRADEGITHUB_HPP
#define _BAZ_TRACINGBAZUPGRADEGITHUB_HPP
#include "curl/HttpClientCurl.hpp"
#include "controllers/AssetSelectorContainsString.hpp"
#include "controllers/AutoUpgraderUnzip.hpp"
#include "controllers/AutoUpgraderLaunch.hpp"
#include "controllers/DownloaderSelective.hpp"
#include "controllers/ReleaseSelectorOrdered.hpp"
#include "controllers/LauncherStd.hpp"
#include "controllers/UnzipperElzip.hpp"
#include "vendors/ApiClientGitHub.hpp"
#include "vendors/ResponseParserGitHub.hpp"
#include "tracing/TracingApiClient.hpp"
#include "tracing/TracingAssetSelector.hpp"
#include "tracing/TracingReleaseSelector.hpp"

struct TracingBazUpgradeGitHub
{
    TracingBazUpgradeGitHub (const std::string& user, const std::string& repo, const std::string& currentVersion, const std::string& assetIdentifier,
    const bool& alphaReleases, const bool& downloadTarBall, const bool& downloadZipBall, const bool& unzipFiles ,const std::string& unzipPath, const std::string& launchPath, std::ostream& stream)
        : apiClient(httpClient, user, repo)
        , tracingApiClient(apiClient, stream)
        , tracingAssetSelector(assetSelector, stream)
        , tracingReleaseSelector(releaseSelector, stream)
        , downloader(tracingApiClient, parser, tracingAssetSelector, tracingReleaseSelector, currentVersion, assetIdentifier, alphaReleases, downloadTarBall, downloadZipBall)
        , upgradeController(downloader, unzipper, unzipFiles, unzipPath)
        , launchController(upgradeController, launcher, launchPath)
    {}

    HTTPClientCurl httpClient;
    ApiClientGitHub apiClient;
    TracingApiClient tracingApiClient;
    ResponseParserGitHub parser;
    AssetSelectorContainsString assetSelector;
    TracingAssetSelector tracingAssetSelector;
    ReleaseSelectorOrdered releaseSelector;
    TracingReleaseSelector tracingReleaseSelector;
    DownloaderSelective downloader;
    UnzipperElzip unzipper;
    LauncherStd launcher;
    AutoUpgraderUnzip upgradeController;
    AutoUpgraderLaunch launchController;

    inline int Upgrade()
    {
        switch (launchController.Upgrade())
        {
            case UpgradeResult::success:
                return 0;
            case UpgradeResult::downloadFailure:
                return -1;
            case UpgradeResult::unpackError:
                return -2;
            case UpgradeResult::launchError:
                return -3;
            default:
                return -4;
        }
    }
};
#endif
