#ifndef _BAZ_AUTOUPGRADER_HPP
#define _BAZ_AUTOUPGRADER_HPP

enum class UpgradeResult : int
{
    success = 0,
    downloadFailure = -1,
    unpackError = -2,
    launchError = -3
};

class AutoUpgrader
{
public:
    AutoUpgrader() = default;
    virtual ~AutoUpgrader() = default;
    AutoUpgrader(const AutoUpgrader& other) = delete;
    AutoUpgrader& operator=(const AutoUpgrader& other) = delete;

    virtual UpgradeResult Upgrade() = 0;
};
#endif
