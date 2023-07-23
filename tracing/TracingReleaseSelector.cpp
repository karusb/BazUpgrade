#include "TracingReleaseSelector.hpp"

TracingReleaseSelector::TracingReleaseSelector(ReleaseSelector& releaseSelector, std::ostream& stream)
    : releaseSelector(releaseSelector)
    , stream(stream)
{}

std::optional<Release> TracingReleaseSelector::SelectEligibleRelease(std::string_view currentVersion, const std::vector<Release> &releases) const
{
    auto selectedRelease = releaseSelector.SelectEligibleRelease(currentVersion, releases);
    if (!selectedRelease)
        stream << "Version up to date! ";
    else if (currentVersion.empty())
        stream << "Latest version: ";
    else
        stream << "Update available, version: ";
    stream << selectedRelease->version << std::endl;
    return selectedRelease;
}

