#ifndef _BAZ_TRACINGRELEASESELECTOR_HPP
#define _BAZ_TRACINGRELEASESELECTOR_HPP
#include "interfaces/ReleaseSelector.hpp"
#include <iostream>

class TracingReleaseSelector
    : public ReleaseSelector
{
public:
    TracingReleaseSelector(ReleaseSelector& releaseSelector, std::ostream& stream);

    virtual std::optional<Release> SelectEligibleRelease(std::string_view currentVersion, const std::vector<Release>& releases) const override;
    
private:
    ReleaseSelector& releaseSelector;
    std::ostream& stream;
};
#endif
