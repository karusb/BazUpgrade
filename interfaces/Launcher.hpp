#ifndef _BAZ_LAUNCHER_HPP
#define _BAZ_LAUNCHER_HPP
#include <string>

class Launcher
{
public:

    Launcher() = default;
    ~Launcher() = default;
    Launcher(const Launcher& other) = delete;
    Launcher& operator=(const Launcher& other) = delete;

    virtual bool Launch(const std::string& launchPath) = 0;
};
#endif
