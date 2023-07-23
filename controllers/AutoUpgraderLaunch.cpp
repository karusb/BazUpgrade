#include "AutoUpgraderLaunch.hpp"

AutoUpgraderLaunch::AutoUpgraderLaunch(AutoUpgrader &autoUpgrader, Launcher &launcher, const std::string &launchPath)
    : autoUpgrader(autoUpgrader)
    , launcher(launcher)
    , launchPath(launchPath)
{}

UpgradeResult AutoUpgraderLaunch::Upgrade()
{
    auto result = autoUpgrader.Upgrade();
    if (result != UpgradeResult::success)
        return result;
    if (!launchPath.empty())
    {
        if (launcher.Launch(launchPath))
            return UpgradeResult::success;
        else
            return UpgradeResult::launchError;
    }
    return UpgradeResult::success;
}
