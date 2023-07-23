#ifndef _BAZ_ASSETSELECTORCONTAINSSTRING_HPP
#define _BAZ_ASSETSELECTORCONTAINSSTRING_HPP
#include "interfaces/AssetSelector.hpp"

class AssetSelectorContainsString
    : public AssetSelector
{
public:
    AssetSelectorContainsString() = default;

    virtual std::optional<Asset> SelectAsset(std::string_view identifier, const std::vector<Asset>& assets) const override;
};
#endif
