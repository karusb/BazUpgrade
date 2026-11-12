#pragma once
#include "util/Json.hpp"

namespace baz
{
Json CreateTestCertificate(const fs::path &publicCertificate);
Json SignWindowsArtifact(const fs::path &artifact, const std::string &thumbprint, const fs::path &signingTool,
                         const std::string &timestampUrl);
} // namespace baz
