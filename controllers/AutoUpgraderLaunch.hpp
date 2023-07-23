#ifndef _BAZ_AUTOUPGRADERLAUNCH_HPP
#define _BAZ_AUTOUPGRADERLAUNCH_HPP
#include "AutoUpgraderUnzip.hpp"
#include "interfaces/Launcher.hpp"

class AutoUpgraderLaunch
    : public AutoUpgrader
{
public:
    AutoUpgraderLaunch(AutoUpgrader& autoUpgrader, Launcher& launcher, const std::string& launchPath);
    ~AutoUpgraderLaunch() = default;

    UpgradeResult Upgrade() override;

private:
    AutoUpgrader &autoUpgrader;
    Launcher& launcher;
    const std::string& launchPath;
};
#endif
