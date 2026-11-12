#pragma once
#include "interfaces/Target.hpp"
#include "interfaces/Transport.hpp"

namespace baz
{
struct InstallResult
{
    bool rebootRequired = false;
    std::string activePath;
};

class Installer
{
  public:
    virtual ~Installer() = default;
    virtual InstallResult Apply(const Target &target, const fs::path &artifact, const fs::path &root,
                                const Cancellation &cancel) = 0;
};
} // namespace baz
