#ifndef _BAZ_RELEASESELECTORORDERED_HPP
#define _BAZ_RELEASESELECTORORDERED_HPP
#include "interfaces/ReleaseSelector.hpp"

class ReleaseSelectorOrdered
    : public ReleaseSelector
{
public:
    ReleaseSelectorOrdered() = default;

    virtual std::optional<Release> SelectEligibleRelease(std::string_view currentVersion, const std::vector<Release>& releases) const override;
};
#endif
