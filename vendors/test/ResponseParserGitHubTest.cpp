#include "gtest/gtest.h"
#include "vendors/ResponseParserGitHub.hpp"
#include "ResponsesGitHub.hpp"

class ResponseParserGitHubTest
    : public ::testing::Test
{
public:
    ResponseParserGitHub parser;
};

TEST_F(ResponseParserGitHubTest, ParseLatestRelease)
{
    std::string response (ResponsesGitHub::latestReleaseResponse);
    std::vector<Release> releases;
    parser.ParseReleases(response, releases);

    EXPECT_EQ(releases.front(), ResponsesGitHub::latestRelease);
}

TEST_F(ResponseParserGitHubTest, ParseReleases_only_alpha)
{
    std::string response (ResponsesGitHub::alphaReleaseOnlyResponse);
    std::vector<Release> releases;
    parser.ParseReleases(response, releases);

    EXPECT_EQ(releases, ResponsesGitHub::alphaReleaseOnly);
}

TEST_F(ResponseParserGitHubTest, ParseReleases_mixed_releases)
{
    std::string response (ResponsesGitHub::mixedReleasesResponse);
    std::vector<Release> releases;
    parser.ParseReleases(response, releases);
    for (auto& release : releases)
        std::cout << release;
    EXPECT_EQ(releases, ResponsesGitHub::mixedReleases);
}
