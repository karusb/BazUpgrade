#ifndef _BAZ_RESPONSEPARSERGITHUB_HPP
#define _BAZ_RESPONSEPARSERGITHUB_HPP
#include "interfaces/ResponseParser.hpp"

class ResponseParserGitHub
    : public ResponseParser
{
public:
    void ParseReleases(const std::string& response, std::vector<Release>& releases) const override;

private:
    std::string_view TarballUrl(std::string_view response) const;
    std::string_view ZipballUrl(std::string_view response) const;
    std::string_view FilenameFromUrl(std::string_view url) const;
    std::string_view TagName(std::string_view response) const;
    std::string_view Name(std::string_view response) const;
    bool Draft(std::string_view response) const;
    bool PreRelease(std::string_view response) const;
    std::vector<std::string_view> AssetUrls(std::string_view response) const;
    std::vector<std::string_view> DivideReleases(std::string_view response) const;
};
#endif
