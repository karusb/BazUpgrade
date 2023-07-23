#ifndef _BAZ_RELEASESELECTORMOCK_HPP
#define _BAZ_RELEASESELECTORMOCK_HPP
#include <string>
#include "gmock/gmock.h"
#include "interfaces/ReleaseSelector.hpp"

class ReleaseSelectorMock
    : public ReleaseSelector
{
public:
    MOCK_CONST_METHOD2(SelectEligibleRelease, std::optional<Release>(std::string_view currentVersion, const std::vector<Release>& releases));
};
#endif
