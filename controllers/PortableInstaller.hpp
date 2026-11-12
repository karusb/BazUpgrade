#pragma once
#include "interfaces/Installer.hpp"

namespace baz
{
class PortableInstaller final : public Installer
{
  public:
    explicit PortableInstaller(std::uint64_t maxBytes = 4ULL * 1024 * 1024 * 1024,
                               std::uint64_t maxEntries = 10000);
    InstallResult Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                        const Cancellation &cancel) override;

  private:
    std::uint64_t maxBytes;
    std::uint64_t maxEntries;
};

void PackDirectory(const fs::path &directory, const fs::path &output);
void SecureDirectory(const fs::path &directory);
} // namespace baz
