#ifndef _BAZ_ASSETSELECTORMOCK_HPP
#define _BAZ_ASSETSELECTORMOCK_HPP
#include <string>
#include "gmock/gmock.h"
#include "interfaces/AssetSelector.hpp"

class AssetSelectorMock
    : public AssetSelector
{
public:
    MOCK_CONST_METHOD2(SelectAsset, std::optional<Asset>(std::string_view identifier, const std::vector<Asset>& assets));
};
#endif
