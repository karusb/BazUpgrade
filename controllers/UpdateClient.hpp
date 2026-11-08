#pragma once
#include "interfaces/Installer.hpp"
#include "tuf/TufClient.hpp"
#include <functional>
#include <memory>
namespace baz
{
struct ClientConfig
{
    std::string repository, appId, channel, os, arch, packageType, target;
    fs::path trustedRoot, stateDirectory, installDirectory;
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
    Json Check(const Cancellation &cancel);
    Json Apply(const Cancellation &cancel, std::uint64_t waitPid = 0,
               const std::function<void()> &ready = {});
    Json VerifyFile(const fs::path &path, const Cancellation &cancel);
    Json Status();
    Json Reconcile(bool installed);
    Json Rollback();

  private:
    Target Resolve(const Cancellation &cancel);
    Json LoadState() const;
    void ValidateState(const Json &state) const;
    ClientConfig config;
    std::unique_ptr<CryptoProvider> crypto;
    std::unique_ptr<Transport> transport;
    std::unique_ptr<TufClient> tuf;
    std::unique_ptr<Installer> installer;
};
Json Capabilities();
} // namespace baz
