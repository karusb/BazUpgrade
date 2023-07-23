#ifndef _BAZ_DOWNLOADERMOCK_HPP
#define _BAZ_DOWNLOADERMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/Downloader.hpp"

class DownloaderMock
    : public Downloader
{
public:
    MOCK_METHOD0(Download, bool());
    MOCK_CONST_METHOD0(LatestVersion, const std::string&());
    MOCK_CONST_METHOD0(LastReleaseAssets, const std::vector<Asset>&());
    MOCK_CONST_METHOD0(SelectedAsset, std::optional<Asset>());
};
#endif
