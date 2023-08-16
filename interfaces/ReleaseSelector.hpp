#ifndef _BAZ_RELEASESELECTOR_HPP
#define _BAZ_RELEASESELECTOR_HPP
#include <vector>
#include <optional>
#include "Release.hpp"

class ReleaseSelector
{
public:

    ReleaseSelector() = default;
    virtual ~ReleaseSelector() = default;
    ReleaseSelector(const ReleaseSelector& other) = delete;
    ReleaseSelector& operator=(const ReleaseSelector& other) = delete;

    virtual std::optional<Release> SelectEligibleRelease(std::string_view currentVersion, const std::vector<Release>& releases) const = 0;
};
#endif
