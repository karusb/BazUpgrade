#include "LauncherStd.hpp"

bool LauncherStd::Launch(const std::string& launchPath)
{
    return !std::system(launchPath.c_str());
}
