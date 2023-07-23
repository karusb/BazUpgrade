#include "AssetSelectorContainsString.hpp"
#include <algorithm>

std::optional<Asset> AssetSelectorContainsString::SelectAsset(std::string_view identifier, const std::vector<Asset>& assets) const
{
    if (identifier.empty())
        return std::nullopt;
    const auto& asset = std::find_if(assets.begin(), assets.end(), [this, &identifier](const Asset& value) -> bool { return value.filename.find(identifier) != std::string::npos; });
    if (asset != std::end(assets))
        return std::optional<Asset>(*asset);
    else 
        return std::nullopt;
}
