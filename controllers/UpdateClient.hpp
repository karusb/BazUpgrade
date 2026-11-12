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
    Target ResolveTrustedTarget(const Cancellation &cancel);
    Json LoadInstallationState() const;
    void ValidateInstallationState(const Json &state) const;

    ClientConfig config;
    std::unique_ptr<CryptoProvider> crypto;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<TufClient> tuf;
    std::unique_ptr<Installer> installer;
};

Json Capabilities();
} // namespace baz
