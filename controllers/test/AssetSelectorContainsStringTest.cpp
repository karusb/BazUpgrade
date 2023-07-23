#include "gtest/gtest.h"
#include "controllers/AssetSelectorContainsString.hpp"
#include <cmath>


class AssetSelectorContainsStringTest
    : public ::testing::Test
{
public:
    AssetSelectorContainsString assetSelector;
};

TEST_F(AssetSelectorContainsStringTest, correct_asset_is_selected)
{
    std::string selector = "selected";
    const Asset asset1 = { "uri", "something-asset.dsc"};
    const Asset asset2 = { "uri", "selected-asset.dsc"};
    const std::vector<Asset> assets {asset1, asset2};

    auto selectedAsset = assetSelector.SelectAsset(selector, assets);
    EXPECT_EQ(*selectedAsset, asset2);
}

TEST_F(AssetSelectorContainsStringTest, empty_selector_returns_no_assets)
{
    std::string selector = "";
    const Asset asset1 = { "uri", "something-asset.dsc"};
    const Asset asset2 = { "uri", "other-asset.dsc"};
    const std::vector<Asset> assets {asset1, asset2};

    auto selectedAsset = assetSelector.SelectAsset(selector, assets);
    EXPECT_EQ(selectedAsset, std::nullopt);
}

TEST_F(AssetSelectorContainsStringTest, no_valid_asset_found)
{
    std::string selector = "selected";
    const Asset asset1 = { "uri", "something-asset.dsc"};
    const Asset asset2 = { "uri", "other-asset.dsc"};
    const std::vector<Asset> assets {asset1, asset2};

    auto selectedAsset = assetSelector.SelectAsset(selector, assets);
    EXPECT_EQ(selectedAsset, std::nullopt);
}

