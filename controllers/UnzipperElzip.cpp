#include "UnzipperElzip.hpp"
#include "elzip.hpp"

bool UnzipperElzip::UnzipAll(const std::string& zipPath, const std::string& targetPath)
{
    bool success = true;
    try
    {
        elz::extractZip(zipPath, targetPath);
    }
    catch(...)
    {
        success = false;
    }
    return success;
}
