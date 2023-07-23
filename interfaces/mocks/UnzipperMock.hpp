#ifndef _BAZ_UNZIPPERMOCK_HPP
#define _BAZ_UNZIPPERMOCK_HPP
#include "gmock/gmock.h"
#include "interfaces/Unzipper.hpp"

class UnzipperMock
    : public Unzipper
{
public:
    MOCK_METHOD2(UnzipAll, bool(const std::string& zipPath, const std::string& targetPath));
};
#endif
