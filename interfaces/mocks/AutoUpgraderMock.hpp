#ifndef _BAZ_AUTOUPGRADERMOCK_HPP
#define _BAZ_AUTOUPGRADERMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/AutoUpgrader.hpp"

class AutoUpgraderMock
    : public AutoUpgrader
{
public:
    MOCK_METHOD0(Upgrade, UpgradeResult());
};
#endif
