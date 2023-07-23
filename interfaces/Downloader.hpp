#ifndef _BAZ_DOWNLOADER_HPP
#define _BAZ_DOWNLOADER_HPP
#include <vector>
#include <optional>
#include <string>
#include "Release.hpp"

class Downloader
{
public:

    Downloader() = default;
    ~Downloader() = default;
    Downloader(const Downloader& other) = delete;
    Downloader& operator=(const Downloader& other) = delete;

    virtual bool Download() = 0;
    virtual const std::string& LatestVersion() const = 0;
    virtual const std::vector<Asset>& LastReleaseAssets() const = 0;
    virtual std::optional<Asset> SelectedAsset() const = 0;
};
#endif
