#pragma once
#include "util/Json.hpp"

namespace baz
{
class RepositoryPublisher
{
  public:
    RepositoryPublisher(fs::path repository, fs::path keys);
    Json Initialize();
    Json Publish(const fs::path &artifact, const std::string &target, const Json &custom);
    Json Refresh();
    Json RestoreMetadata();
    Json RotateRoot();
    Json ExportElectron(const std::string &target, const std::string &baseUrl, const fs::path &output);
    Json ExportAppInstaller(const std::string &target, const std::string &baseUrl, const fs::path &output);

  private:
    Json ReadSignedMetadata(const std::string &role);
    Json CommitTargets(Json body);
    void WriteOnlineMetadata(const Json &checkpoint);
    std::vector<fs::path> GetRoleSigningKeys(const Json &ring, const std::string &role) const;

    fs::path repository;
    fs::path keys;
};
} // namespace baz
