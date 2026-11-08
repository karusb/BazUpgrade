#pragma once

#include "interfaces/CryptoProvider.hpp"

namespace baz
{
class OpenSslCrypto final : public CryptoProvider
{
  public:
    std::string Hash(const std::string &bytes, const std::string &algorithm = "sha256") const override;
    std::string HashFile(const fs::path &path, const std::string &algorithm = "sha256") const override;
    bool Verify(const Json &key, const std::string &message, const std::string &signature) const override;
};

std::string Hex(const unsigned char *bytes, std::size_t size);
std::string Unhex(const std::string &value);
Json GenerateEd25519Key(const fs::path &privateKey);
Json Sign(const Json &signedValue, const std::vector<fs::path> &keys);
} // namespace baz
