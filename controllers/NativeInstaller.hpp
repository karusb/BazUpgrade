#pragma once
#include "interfaces/Installer.hpp"

namespace baz
{
class NativeInstaller final : public Installer
{
  public:
    static bool Supports(const std::string &kind);
    InstallResult Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                        const Cancellation &cancel) override;
};

void WaitForProcess(std::uint64_t pid, const Cancellation &cancel);
} // namespace baz
