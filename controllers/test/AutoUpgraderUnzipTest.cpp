#include "gtest/gtest.h"
#include "controllers/AutoUpgraderUnzip.hpp"
#include "interfaces/mocks/DownloaderMock.hpp"
#include "interfaces/mocks/UnzipperMock.hpp"
#include <cmath>


class AutoUpgraderUnzipTest
    : public ::testing::Test
{
public:
    ::testing::StrictMock<DownloaderMock> downloader;
    ::testing::StrictMock<UnzipperMock> unzipper;
};

TEST_F(AutoUpgraderUnzipTest, nothing_happens_on_download_failure)
{
    std::string unzipPath {"."};
    bool unzip = true;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(false));
    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::downloadFailure);
}

TEST_F(AutoUpgraderUnzipTest, unzip_false_only_downloads)
{
    std::string unzipPath {"."};
    bool unzip = false;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(true));
    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::success);
}

TEST_F(AutoUpgraderUnzipTest, download_and_unzip_selected_asset)
{
    std::string unzipPath {"."};
    bool unzip = true;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };
    auto asset = std::optional<Asset>({"uri", "something.zip"});

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(true));
    EXPECT_CALL(downloader, SelectedAsset).WillOnce(testing::Return(asset));
    EXPECT_CALL(unzipper, UnzipAll(asset->filename, unzipPath)).WillOnce(testing::Return(true));
    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::success);
}

TEST_F(AutoUpgraderUnzipTest, download_and_unzip_all_assets)
{
    std::string unzipPath {"."};
    bool unzip = true;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };
    std::vector<Asset> assets = {{"url", "something.zip"}, {"url2", "another.zip"}, {"url3", "together.zip"}};

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(true));
    EXPECT_CALL(downloader, SelectedAsset).WillOnce(testing::Return(std::nullopt));
    EXPECT_CALL(downloader, LastReleaseAssets).WillOnce(testing::ReturnRef(assets));
    for (auto& asset : assets)
        EXPECT_CALL(unzipper, UnzipAll(asset.filename, unzipPath)).WillOnce(testing::Return(true));

    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::success);
}

TEST_F(AutoUpgraderUnzipTest, unzip_failure_returns_error)
{
    std::string unzipPath {"."};
    bool unzip = true;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };
    auto asset = std::optional<Asset>({"uri", "something.zip"});

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(true));
    EXPECT_CALL(downloader, SelectedAsset).WillOnce(testing::Return(asset));
    EXPECT_CALL(unzipper, UnzipAll(asset->filename, unzipPath)).WillOnce(testing::Return(false));
    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::unpackError);
}

TEST_F(AutoUpgraderUnzipTest, unzip_failure_on_all_assets_returns_error)
{
    std::string unzipPath {"."};
    bool unzip = true;
    AutoUpgraderUnzip upgraderUnzip { downloader, unzipper, unzip, unzipPath };
    std::vector<Asset> assets = {{"url", "something.zip"}, {"url2", "another.zip"}};

    EXPECT_CALL(downloader, Download).WillOnce(testing::Return(true));
    EXPECT_CALL(downloader, SelectedAsset).WillOnce(testing::Return(std::nullopt));
    EXPECT_CALL(downloader, LastReleaseAssets).WillOnce(testing::ReturnRef(assets));
    EXPECT_CALL(unzipper, UnzipAll(assets.at(0).filename, unzipPath)).WillOnce(testing::Return(true));
    EXPECT_CALL(unzipper, UnzipAll(assets.at(1).filename, unzipPath)).WillOnce(testing::Return(false));

    EXPECT_EQ(upgraderUnzip.Upgrade(), UpgradeResult::unpackError);
}