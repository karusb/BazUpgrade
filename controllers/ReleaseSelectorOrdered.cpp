#include "ReleaseSelectorOrdered.hpp"
#include <algorithm>

std::optional<Release> ReleaseSelectorOrdered::SelectEligibleRelease(std::string_view currentVersion, const std::vector<Release>& releases) const
{
    const auto& latestReleaseResult = std::find_if(releases.begin(), releases.end(), [currentVersion](const Release& release) -> bool { return currentVersion < release.version;});
    if (latestReleaseResult != std::end(releases))
        return std::optional<Release>(*latestReleaseResult);
    return std::nullopt;
}
