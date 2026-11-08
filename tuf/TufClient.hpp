#pragma once

#include "interfaces/Target.hpp"
#include "interfaces/Transport.hpp"
#include "tuf/Metadata.hpp"
#include <map>
#include <set>

namespace baz
{
class TufClient
{
  public:
    TufClient(Transport &transport, const CryptoProvider &crypto, fs::path trustedRoot, fs::path state);
    Target Resolve(const std::string &targetPath, const Cancellation &cancel);
    void Download(const Target &target, const fs::path &destination, const Cancellation &cancel);

  private:
    Json FetchMetadata(const std::string &filename, const Json *description, const Cancellation &cancel);
    void VerifyMetadata(const Json &envelope, const std::string &type, const std::string &cacheName);
    void VerifyDescription(const std::string &bytes, const Json &description) const;
    std::optional<Target> Search(const Json &metadata, const std::string &path, const Cancellation &cancel,
                                 std::set<std::string> &visited, unsigned depth, bool &terminated);
    void Refresh(const Cancellation &cancel);
    Transport &transport;
    const CryptoProvider &crypto;
    fs::path bootstrap;
    fs::path state;
    Json root;
    Json snapshot;
    Json targets;
    std::int64_t started = 0;
    unsigned delegatedCount = 0;
};
} // namespace baz
