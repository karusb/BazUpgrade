#ifndef _BAZ_UNZIPPERELZIP_HPP
#define _BAZ_UNZIPPERELZIP_HPP
#include <string>
#include "interfaces/Unzipper.hpp"

class UnzipperElzip
    : public Unzipper
{
public:
    UnzipperElzip() = default;
    ~UnzipperElzip() = default;

    bool UnzipAll(const std::string& zipPath, const std::string& targetPath) override;
};
#endif
