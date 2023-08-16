#ifndef _BAZ_LAUNCHERSTD_HPP
#define _BAZ_LAUNCHERSTD_HPP
#include "interfaces/Launcher.hpp"

class LauncherStd
    : public Launcher
{
public:
    LauncherStd() = default;

    bool Launch(const std::string& launchPath) override;
};
#endif
