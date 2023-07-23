#ifndef _BAZ_LAUNCHERMOCK_HPP
#define _BAZ_LAUNCHERMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/HttpClient.hpp"

class LauncherMock
    : public Launcher
{
public:
    MOCK_METHOD1(Launch, bool(const std::string& launchPath));
};
#endif
