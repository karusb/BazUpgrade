#ifndef _BAZ_TRACINGASSETSELECTOR_HPP
#define _BAZ_TRACINGASSETSELECTOR_HPP
#include "interfaces/AssetSelector.hpp"
#include <iostream>

class TracingAssetSelector
    : public AssetSelector
{
public:
    TracingAssetSelector(AssetSelector& assetSelector, std::ostream& stream);

    virtual std::optional<Asset> SelectAsset(std::string_view identifier, const std::vector<Asset>& assets) const override;
    
private:
    AssetSelector& assetSelector;
    std::ostream& stream;
};
#endif
