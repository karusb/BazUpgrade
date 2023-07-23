#ifndef _BAZ_RELEASE_HPP
#define _BAZ_RELEASE_HPP
#include <vector>
#include <string>
#include <iostream>
#include "Asset.hpp"

struct Release
{
    Release() = default;
    bool operator== (const Release &c) const
    {
        return (version == c.version && name == c.name && assets == c.assets && tarball == c.tarball && zipball == c.zipball && preRelease == c.preRelease  && draft == c.draft);
    }
    bool operator!= (const Release &c) const { return !(*this == c); }
    friend std::ostream& operator<<(std::ostream& os, const Release& release)
    {
        os << "-----------------------------" << std::endl;
        os << "VERSION: " << release.version << std::endl;
        os << "NAME: " << release.name << std::endl;
        os << "DRAFT: " << release.draft << std::endl;
        os << "PRERELEASE: " << release.preRelease << std::endl;
        os << "TARBALL: " << release.tarball.url << std::endl;
        os << "ZIPBALL: " << release.zipball.url << std::endl;
        os << "ASSETS: " << std::endl;
        for (const auto& asset : release.assets)
            os << "    " << asset.filename << " url: " << asset.url << std::endl;
        os << "-----------------------------" << std::endl;
        return os;
    }

    std::string version;
    std::string name;
    std::vector<Asset> assets;
    Asset tarball;
    Asset zipball;
    bool preRelease;
    bool draft;
};
#endif
