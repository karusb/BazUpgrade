#ifndef _BAZ_ASSETSELECTOR_HPP
#define _BAZ_ASSETSELECTOR_HPP
#include <vector>
#include <optional>
#include "Asset.hpp"

class AssetSelector
{
public:

    AssetSelector() = default;
    ~AssetSelector() = default;
    AssetSelector(const AssetSelector& other) = delete;
    AssetSelector& operator=(const AssetSelector& other) = delete;

    virtual std::optional<Asset> SelectAsset(std::string_view identifier, const std::vector<Asset>& assets) const = 0;
};
#endif
