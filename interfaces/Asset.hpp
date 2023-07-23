#ifndef _BAZ_ASSET_HPP
#define _BAZ_ASSET_HPP
#include <string>

struct Asset
{
    Asset() = default;
    bool operator== (const Asset &c) const { return (url == c.url && filename == c.filename); }
    bool operator!= (const Asset &c) const { return !(*this == c); }

    std::string url;
    std::string filename;
};
#endif
