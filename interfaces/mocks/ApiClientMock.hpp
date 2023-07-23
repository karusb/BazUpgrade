#ifndef _BAZ_APICLIENTMOCK_HPP
#define _BAZ_APICLIENTMOCK_HPP
#include <string>
#include "gmock/gmock.h"
#include "interfaces/ApiClient.hpp"

class ApiClientMock
    : public ApiClient
{
public:
    MOCK_METHOD1(GetLatestRelease, bool(std::string& response));
    MOCK_METHOD1(GetReleases, bool(std::string& response));
    MOCK_METHOD2(GetRelease, bool(std::string& response, const std::string releaseId));
    MOCK_METHOD1(DownloadAsset, bool(const Asset&));
};
#endif
