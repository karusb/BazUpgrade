#pragma once

#include "util/Json.hpp"

namespace baz
{
class CryptoProvider
{
  public:
    virtual ~CryptoProvider() = default;
    virtual std::string Hash(const std::string &bytes, const std::string &algorithm = "sha256") const = 0;
    virtual std::string HashFile(const fs::path &path, const std::string &algorithm = "sha256") const = 0;
    virtual bool Verify(const Json &key, const std::string &message, const std::string &signature) const = 0;
};
} // namespace baz
