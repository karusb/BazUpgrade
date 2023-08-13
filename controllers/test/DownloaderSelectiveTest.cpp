#include "gtest/gtest.h"
#include "controllers/DownloaderSelective.hpp"
#include "interfaces/mocks/ApiClientMock.hpp"
#include "interfaces/mocks/AssetSelectorMock.hpp"
#include "interfaces/mocks/ResponseParserMock.hpp"
#include "interfaces/mocks/ReleaseSelectorMock.hpp"
#include <cmath>
#include <string>


class DownloaderSelectiveTest
    : public ::testing::Test
{
public:
    std::string currentVersion {"v1.0"};
    std::string assetIdentifier {""};
    bool alphaReleases = false;
    bool downloadTarball = false;
    bool downloadZipball = false;
    ::testing::StrictMock<ApiClientMock> apiClient;
    ::testing::StrictMock<ResponseParserMock> parser;
    ::testing::StrictMock<AssetSelectorMock> assetSelector;
    ::testing::StrictMock<ReleaseSelectorMock> releaseSelector;
    DownloaderSelective downloader { apiClient, parser, assetSelector, releaseSelector, currentVersion, assetIdentifier, alphaReleases, downloadTarball, downloadZipball };
    Release latestRelease 
    {"v1.1", "Software v1.1", 
        {
        {"https://uri.com/r.zip", "r.zip"},
        {"https://uri.com/z.zip", "z.zip"} 
        },
    {"https://uri.com/tar/1.1", "1.1.tar.gz",}, {"https://uri.com/zip/1.1", "1.1.zip"}, false, false};
    Release alphaRelease 
    {"v1.2", "Software v1.2", 
        {
        {"https://uri.com/r.zip", "r.zip"},
        {"https://uri.com/z.zip", "z.zip"} 
        },
    {"https://uri.com/tar/v1.2", "v1.2.tar.gz",}, {"https://uri.com/zip/v1.2", "v1.2.zip"}, true, false};
    Release noAssetRelease 
    {"v1.3", "Software v1.3", 
    {},
    {"https://uri.com/tar/v1.3", "v1.3.tar.gz",}, {"https://uri.com/zip/v1.3", "v1.3.zip"}, false, false};


    void ExpectSelectReleaseAndAsset(std::optional<Release> release, std::optional<Asset> asset)
    {
        EXPECT_CALL(parser, ParseReleases(testing::_, testing::_));
        EXPECT_CALL(releaseSelector, SelectEligibleRelease(std::string_view(currentVersion), testing::_)).WillOnce(testing::Return(release));
        EXPECT_CALL(assetSelector, SelectAsset(std::string_view(assetIdentifier), testing::_)).WillOnce(testing::Return(asset));
    }

    template <typename Arg>
    void MultiPushback(std::vector<Arg>& storage, Arg& arg)
    {
        storage.push_back(arg);
    }
    template <typename Arg, typename ...Args>
    void MultiPushback(std::vector<Arg>& storage, Arg& arg, Args&... args)
    {
        MultiPushback(storage, arg);
        MultiPushback(storage, args...);
    }
    template <typename ...Args>
    void MakeReleaseAvailabe(Args&... args) 
    {
        EXPECT_CALL(parser, ParseReleases(testing::_, testing::_)).WillOnce(testing::WithArg<1>([&](std::vector<Release>& releases){ MultiPushback(releases, args...);}));
    }
};

TEST_F(DownloaderSelectiveTest, download_latest_release_assets)
{
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(latestRelease, std::nullopt);

    for (const auto& asset : latestRelease.assets)
        EXPECT_CALL(apiClient, DownloadAsset(asset)).WillOnce(testing::Return(true));

    EXPECT_TRUE(downloader.Download());
    EXPECT_EQ(downloader.LatestVersion(), latestRelease.version);
    EXPECT_EQ(downloader.LastReleaseAssets(), latestRelease.assets);
    EXPECT_EQ(downloader.SelectedAsset(), std::nullopt);
}

TEST_F(DownloaderSelectiveTest, no_eligible_release_results_in_success_and_no_download)
{
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(std::nullopt, std::nullopt);

    EXPECT_TRUE(downloader.Download());
}

TEST_F(DownloaderSelectiveTest, failed_get_latest_release_fails_download)
{
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(false));
    EXPECT_FALSE(downloader.Download());
}

TEST_F(DownloaderSelectiveTest, failed_get_releases_fails_download)
{
    alphaReleases = true;
    EXPECT_CALL(apiClient, GetReleases(testing::_)).WillOnce(testing::Return(false));
    EXPECT_FALSE(downloader.Download());
}

TEST_F(DownloaderSelectiveTest, download_selected_latest_release_asset)
{
    assetIdentifier = "z.zip";
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(latestRelease, latestRelease.assets.at(1));
    EXPECT_CALL(apiClient, DownloadAsset(latestRelease.assets.at(1))).WillOnce(testing::Return(true));

    EXPECT_TRUE(downloader.Download());
    EXPECT_EQ(downloader.LatestVersion(), latestRelease.version);
    EXPECT_EQ(downloader.LastReleaseAssets(), latestRelease.assets);
    EXPECT_EQ(downloader.SelectedAsset(), latestRelease.assets.at(1));
}

TEST_F(DownloaderSelectiveTest, invalid_selector_reports_false_on_download)
{
    assetIdentifier = "kek.zip";
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(latestRelease, std::nullopt);

    EXPECT_FALSE(downloader.Download());
    EXPECT_EQ(downloader.SelectedAsset(), std::nullopt);
}

TEST_F(DownloaderSelectiveTest, download_alpha_release_asset_when_enabled)
{
    assetIdentifier = "z.zip";
    alphaReleases = true;
    EXPECT_CALL(apiClient, GetReleases(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(alphaRelease, alphaRelease.assets.at(1));
    EXPECT_CALL(apiClient, DownloadAsset(alphaRelease.assets.at(1))).WillOnce(testing::Return(true));

    EXPECT_TRUE(downloader.Download());
    EXPECT_EQ(downloader.LatestVersion(), alphaRelease.version);
    EXPECT_EQ(downloader.LastReleaseAssets(), alphaRelease.assets);
    EXPECT_EQ(downloader.SelectedAsset(), alphaRelease.assets.at(1));
}

TEST_F(DownloaderSelectiveTest, no_download_when_newest_release_has_no_asset)
{
    alphaReleases = true;
    EXPECT_CALL(apiClient, GetReleases(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(noAssetRelease, std::nullopt);
    EXPECT_TRUE(downloader.Download());
    EXPECT_EQ(downloader.LastReleaseAssets(), noAssetRelease.assets);
    EXPECT_EQ(downloader.SelectedAsset(), std::nullopt);
}

TEST_F(DownloaderSelectiveTest, download_tarball_zipball_on_enabled)
{
    downloadTarball = true;
    downloadZipball = true;
    EXPECT_CALL(apiClient, GetLatestRelease(testing::_)).WillOnce(testing::Return(true));
    ExpectSelectReleaseAndAsset(noAssetRelease, std::nullopt);

    EXPECT_CALL(apiClient, DownloadAsset(noAssetRelease.zipball)).WillOnce(testing::Return(true));
    EXPECT_CALL(apiClient, DownloadAsset(noAssetRelease.tarball)).WillOnce(testing::Return(true));

    EXPECT_TRUE(downloader.Download());
}
