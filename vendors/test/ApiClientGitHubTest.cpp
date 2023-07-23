#include "gtest/gtest.h"
#include "vendors/ApiClientGitHub.hpp"
#include "interfaces/mocks/HttpClientMock.hpp"
#include <cmath>


class ApiClientGitHubTest
    : public ::testing::Test
{
public:
    std::string user {"user"};
    std::string repo {"repo"};
    ::testing::StrictMock<HTTPClientMock> httpClient;
    ApiClientGitHub apiClient { httpClient, user, repo };
    std::string response {"some response"};
    std::string notFoundResponse {"\"message\":\"Not Found\""};
};

TEST_F(ApiClientGitHubTest, GetLatestRelease)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/latest";
    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(true));
    EXPECT_TRUE(apiClient.GetLatestRelease(response));

    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(false));
    EXPECT_FALSE(apiClient.GetLatestRelease(response));
}

TEST_F(ApiClientGitHubTest, GetLatestRelease_not_found)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/latest";
    EXPECT_CALL(httpClient, Get(url, notFoundResponse)).WillOnce(testing::Return(true));
    EXPECT_FALSE(apiClient.GetLatestRelease(notFoundResponse));
}

TEST_F(ApiClientGitHubTest, GetReleases)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases";
    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(true));
    EXPECT_TRUE(apiClient.GetReleases(response));

    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(false));
    EXPECT_FALSE(apiClient.GetReleases(response));
}

TEST_F(ApiClientGitHubTest, GetReleases_not_found)
{
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases";
    EXPECT_CALL(httpClient, Get(url, notFoundResponse)).WillOnce(testing::Return(true));
    EXPECT_FALSE(apiClient.GetReleases(notFoundResponse));
}

TEST_F(ApiClientGitHubTest, GetRelease)
{
    std::string releaseId = "40605006";
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/" + releaseId;
    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(true));
    EXPECT_TRUE(apiClient.GetRelease(response, releaseId));

    EXPECT_CALL(httpClient, Get(url, response)).WillOnce(testing::Return(false));
    EXPECT_FALSE(apiClient.GetRelease(response, releaseId));
}

TEST_F(ApiClientGitHubTest, GetRelease_not_found)
{
    std::string releaseId = "40605006";
    std::string url = "https://api.github.com/repos/" + user + "/" + repo + "/releases/" + releaseId;
    EXPECT_CALL(httpClient, Get(url, notFoundResponse)).WillOnce(testing::Return(true));
    EXPECT_FALSE(apiClient.GetRelease(notFoundResponse, releaseId));
}

TEST_F(ApiClientGitHubTest, DownloadAsset)
{
    Asset asset = { "https://test.com/testfile.zip", "testfile.zip" };
    EXPECT_CALL(httpClient, Download(asset.url, testing::_)).WillOnce(testing::Return(true));
    EXPECT_TRUE(apiClient.DownloadAsset(asset));

    EXPECT_CALL(httpClient, Download(asset.url, testing::_)).WillOnce(testing::Return(false));
    EXPECT_FALSE(apiClient.DownloadAsset(asset));
}