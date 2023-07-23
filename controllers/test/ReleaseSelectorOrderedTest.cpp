#include "gtest/gtest.h"
#include "controllers/ReleaseSelectorOrdered.hpp"
#include <cmath>


class ReleaseSelectorOrderedTest
    : public ::testing::Test
{
public:
    ReleaseSelectorOrdered releaseSelector;
    Release currentRelease {"v1.0", "Software v1.0",  { {},{} },{"",}, {"", ""}, false, false};
    Release latestRelease {"v1.1", "Software v1.1",  { {},{} },{"",}, {"", ""}, false, false};
    std::vector<Release> releases { latestRelease, currentRelease };
};

TEST_F(ReleaseSelectorOrderedTest, correct_release_is_selected)
{
    std::string version = currentRelease.version;
    auto selectedRelease = releaseSelector.SelectEligibleRelease(version, releases);
    EXPECT_EQ(*selectedRelease, latestRelease);
}

TEST_F(ReleaseSelectorOrderedTest, when_on_latest_version_returns_null_opt)
{
    std::string version = latestRelease.version;
    auto selectedRelease = releaseSelector.SelectEligibleRelease(version, releases);
    EXPECT_EQ(selectedRelease, std::nullopt);
}

TEST_F(ReleaseSelectorOrderedTest, when_on_higher_version_returns_null_opt)
{
    std::string version = "v2.0";
    auto selectedRelease = releaseSelector.SelectEligibleRelease(version, releases);
    EXPECT_EQ(selectedRelease, std::nullopt);
}


