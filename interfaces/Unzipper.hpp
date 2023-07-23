#ifndef _BAZ_UNZIPPER_HPP
#define _BAZ_UNZIPPER_HPP
#include <string>

class Unzipper
{
public:
    Unzipper() = default;
    ~Unzipper() = default;
    Unzipper(const Unzipper& other) = delete;
    Unzipper& operator=(const Unzipper& other) = delete;

    virtual bool UnzipAll(const std::string& zipPath, const std::string& targetPath) = 0;
};
#endif
