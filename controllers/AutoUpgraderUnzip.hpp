#ifndef _BAZ_AUTOUPGRADERUNZIP_HPP
#define _BAZ_AUTOUPGRADERUNZIP_HPP
#include "interfaces/AutoUpgrader.hpp"
#include "interfaces/Downloader.hpp"
#include "interfaces/Unzipper.hpp"

class AutoUpgraderUnzip
    : public AutoUpgrader
{
public:
    AutoUpgraderUnzip(Downloader& downloader, Unzipper& unzipper, bool unzipFiles, const std::string& unzipPath);
    ~AutoUpgraderUnzip() = default;

    UpgradeResult Upgrade() override;

private:
    bool ContainsZipExtension(const std::string_view& file) const;
    std::optional<bool> UnzipSelectedAsset();
    bool UnzipLastReleaseAssets();

    Downloader& downloader;
    Unzipper& unzipper;
    bool unzipFiles;
    const std::string& unzipPath;
};
#endif
