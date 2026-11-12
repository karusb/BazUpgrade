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
    Json BuildPublicationCheckpoint(Json body);
    void WriteOnlineMetadata(const Json &checkpoint);
    std::vector<fs::path> GetRoleSigningKeys(const Json &ring, const std::string &role) const;
    void GenerateRoleKeys(Json &root, Json &ring, const std::string &role, std::uint64_t version,
                          unsigned count, unsigned threshold) const;
    static void ValidateReleaseDescriptor(const Json &custom);
    static fs::path GetHashedTargetPath(const fs::path &target, const std::string &hash);
    static std::string BuildFeedUrl(const std::string &baseUrl, const std::string &path);
    fs::path CopyPublishedArtifact(const fs::path &artifact, const fs::path &target,
                                   const std::string &hash) const;

    fs::path repository;
    fs::path keys;
};
} // namespace baz
