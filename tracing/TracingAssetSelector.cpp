#include "TracingAssetSelector.hpp"

TracingAssetSelector::TracingAssetSelector(AssetSelector& assetSelector, std::ostream& stream)
    : assetSelector(assetSelector)
    , stream(stream)
{}

std::optional<Asset> TracingAssetSelector::SelectAsset(std::string_view identifier, const std::vector<Asset> &assets) const
{
    auto selectedAsset = assetSelector.SelectAsset(identifier, assets);
    if (!selectedAsset)
        stream << "No assets found matching with identifier " << identifier << std::endl;
    else
        stream << "Selected asset file " << selectedAsset->filename << std::endl;
    return selectedAsset;
}
