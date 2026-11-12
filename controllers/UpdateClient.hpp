#pragma once
#include "interfaces/Installer.hpp"
#include "tuf/TufClient.hpp"
#include <functional>
#include <memory>

namespace baz
{
struct ClientConfig
{
    std::string repository;
    std::string appId;
    std::string channel;
    std::string os;
    std::string arch;
    std::string packageType;
    std::string target;
    fs::path trustedRoot;
    fs::path stateDirectory;
    fs::path installDirectory;
    bool allowLoopbackHttp = false;
    std::uint64_t maxDownloadBytes = 2ULL * 1024 * 1024 * 1024;
    std::uint64_t maxExtractedBytes = 4ULL * 1024 * 1024 * 1024;

    static ClientConfig Load(const fs::path &path);
};

class UpdateClient
{
  public:
    explicit UpdateClient(ClientConfig config);
    UpdateClient(ClientConfig config, std::unique_ptr<CryptoProvider> crypto,
                 std::unique_ptr<Transport> transport, std::unique_ptr<Installer> installer);

    Json CheckForUpdate(const Cancellation &cancel);
    Json ApplyUpdate(const Cancellation &cancel, std::uint64_t waitPid = 0,
                     const std::function<void()> &ready = {});
    Json VerifyDownloadedFile(const fs::path &path, const Cancellation &cancel);
    Json ReadInstallationState();
    Json ReconcileInstallationOutcome(bool installed);
    Json RollbackPortableInstallation();

  private:
    void ValidateConfiguration() const;
    void InitializeBackends();
    Target ResolveTrustedTarget(const Cancellation &cancel);
    void ValidateTargetPolicy(const Target &target) const;
    void RecordTrustedTarget(const Target &target);
    Json LoadInstallationState() const;
    void ValidateInstallationState(const Json &state) const;
    void SaveInstallationState(const Json &state) const;

    void EnsureInstallationOwnership() const;
    bool IsInstalledRelease(const Target &target, const Json &state) const;
    fs::path GetArtifactCachePath(const Target &target) const;
    void RecordPendingDownload(Json &state, const Target &target) const;
    void DownloadOrVerifyCachedArtifact(const Target &target, const fs::path &artifact,
                                        const Cancellation &cancel);
    static void NotifyReadyAndWaitForExit(const Cancellation &cancel, std::uint64_t waitPid,
                                          const std::function<void()> &ready);
    void RecordCompletedInstallation(Json &state, const Target &target, const InstallResult &result) const;

    void AcceptPendingInstallation(Json &state) const;
    void DiscardPendingInstallation(const Json &state) const;
    void RemovePartialDownloads(const std::string &hash) const;
    void RemoveInactiveStaging(const Json &state, const std::string &hash) const;

    ClientConfig config;
    std::unique_ptr<CryptoProvider> crypto;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<TufClient> tuf;
    std::unique_ptr<Installer> installer;
};

Json Capabilities();
} // namespace baz
