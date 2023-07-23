#ifndef _BAZ_BAZUPGRADEGITHUB_HPP
#define _BAZ_BAZUPGRADEGITHUB_HPP
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

struct BazUpgradeGitHub
{
    BazUpgradeGitHub (const std::string& user, const std::string& repo, const std::string& currentVersion, const std::string& assetIdentifier,
    const bool& alphaReleases, const bool& downloadTarBall, const bool& downloadZipBall, const bool& unzipFiles ,const std::string& unzipPath, const std::string& launchPath)
        : apiClient(httpClient, user, repo)
        , downloader(apiClient, parser, assetSelector, releaseSelector, currentVersion, assetIdentifier, alphaReleases, downloadTarBall, downloadZipBall)
        , upgradeController(downloader, unzipper, unzipFiles, unzipPath)
        , launchController(upgradeController, launcher, launchPath)
    {}

    HTTPClientCurl httpClient;
    ApiClientGitHub apiClient;
    ResponseParserGitHub parser;
    AssetSelectorContainsString assetSelector;
    ReleaseSelectorOrdered releaseSelector;
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
