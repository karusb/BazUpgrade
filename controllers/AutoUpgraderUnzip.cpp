#include "AutoUpgraderUnzip.hpp"
#include <iostream>

AutoUpgraderUnzip::AutoUpgraderUnzip(Downloader& downloader, Unzipper& unzipper, bool unzipFiles, const std::string& unzipPath)
    : downloader(downloader)
    , unzipper(unzipper)
    , unzipFiles(unzipFiles)
    , unzipPath(unzipPath)
{}

UpgradeResult AutoUpgraderUnzip::Upgrade()
{
    if (!downloader.Download())
        return UpgradeResult::downloadFailure;

    if (unzipFiles)
    {
        auto unzippedAsset = UnzipSelectedAsset();
        if (unzippedAsset && *unzippedAsset)
            return UpgradeResult::success;
        else if (unzippedAsset && !*unzippedAsset)
            return UpgradeResult::unpackError;
        else if (UnzipLastReleaseAssets())
            return UpgradeResult::success;
        else 
            return UpgradeResult::unpackError;
    }
    else 
        return UpgradeResult::success;
}

bool AutoUpgraderUnzip::ContainsZipExtension(const std::string &file) const
{
    return file.find(".zip") != std::string::npos;
}

std::optional<bool> AutoUpgraderUnzip::UnzipSelectedAsset()
{
    auto asset = downloader.SelectedAsset();
    if (asset && ContainsZipExtension(asset->filename))
        return unzipper.UnzipAll(asset->filename, ".");
    return std::nullopt;
}

bool AutoUpgraderUnzip::UnzipLastReleaseAssets()
{
    auto& assets = downloader.LastReleaseAssets();
    for (auto& asset : assets)
        if (ContainsZipExtension(asset.filename))
            if (!unzipper.UnzipAll(asset.filename, unzipPath))
                return false;
    return true;
}
