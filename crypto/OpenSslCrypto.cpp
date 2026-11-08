#include "crypto/OpenSslCrypto.hpp"
#include <fstream>
#include <memory>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace baz
{
namespace
{
using Key = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>;
using Digest = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

const EVP_MD *Algorithm(const std::string &name)
{
    Require(name == "sha256" || name == "sha512", ErrorCode::trust, "Unsupported hash algorithm");
    return name == "sha256" ? EVP_sha256() : EVP_sha512();
}

Key PrivateKey(const fs::path &path)
{
    auto pem = ReadFile(path, 65536);
    BIO *bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    Require(bio != nullptr, ErrorCode::io, "Cannot load signing key");
    auto noPassword = [](char *, int, int, void *) -> int { return 0; };
    Key key(PEM_read_bio_PrivateKey(bio, nullptr, noPassword, nullptr), EVP_PKEY_free);
    BIO_free(bio);
    Require(key && EVP_PKEY_id(key.get()) == EVP_PKEY_ED25519, ErrorCode::trust,
            "Expected Ed25519 signing key");
    return key;
}

Json PublicKey(EVP_PKEY *key)
{
    unsigned char bytes[32];
    std::size_t length = sizeof(bytes);
    Require(EVP_PKEY_get_raw_public_key(key, bytes, &length) == 1 && length == 32, ErrorCode::trust,
            "Cannot encode public key");
    return {{"keytype", "ed25519"}, {"scheme", "ed25519"}, {"keyval", {{"public", Hex(bytes, length)}}}};
}
} // namespace

std::string Hex(const unsigned char *bytes, std::size_t size)
{
    static constexpr char alphabet[] = "0123456789abcdef";
    std::string out(size * 2, '0');
    for (std::size_t i = 0; i < size; ++i)
    {
        out[2 * i] = alphabet[bytes[i] >> 4];
        out[2 * i + 1] = alphabet[bytes[i] & 15];
    }
    return out;
}

std::string Unhex(const std::string &value)
{
    Require(value.size() % 2 == 0, ErrorCode::trust, "Invalid hexadecimal value");
    auto nibble = [](char c) -> unsigned char
    {
        if (c >= '0' && c <= '9')
            return static_cast<unsigned char>(c - '0');
        if (c >= 'a' && c <= 'f')
            return static_cast<unsigned char>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F')
            return static_cast<unsigned char>(c - 'A' + 10);
        throw Error(ErrorCode::trust, "Invalid hexadecimal value");
    };
    std::string out(value.size() / 2, '\0');
    for (std::size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>((nibble(value[2 * i]) << 4) | nibble(value[2 * i + 1]));
    return out;
}

std::string OpenSslCrypto::Hash(const std::string &bytes, const std::string &algorithm) const
{
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    Require(EVP_Digest(bytes.data(), bytes.size(), digest, &length, Algorithm(algorithm), nullptr) == 1,
            ErrorCode::trust, "Hash operation failed");
    return Hex(digest, length);
}

std::string OpenSslCrypto::HashFile(const fs::path &path, const std::string &algorithm) const
{
    Digest context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    Require(context && EVP_DigestInit_ex(context.get(), Algorithm(algorithm), nullptr) == 1, ErrorCode::trust,
            "Hash initialization failed");
    std::ifstream file(path, std::ios::binary);
    Require(file.good(), ErrorCode::io, "Cannot open artifact");
    char buffer[65536];
    while (file.read(buffer, sizeof(buffer)) || file.gcount())
        Require(EVP_DigestUpdate(context.get(), buffer, static_cast<std::size_t>(file.gcount())) == 1,
                ErrorCode::trust, "Hash operation failed");
    Require(file.eof(), ErrorCode::io, "Artifact read failed");
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    Require(EVP_DigestFinal_ex(context.get(), digest, &length) == 1, ErrorCode::trust,
            "Hash operation failed");
    return Hex(digest, length);
}

bool OpenSslCrypto::Verify(const Json &key, const std::string &message, const std::string &signature) const
{
    Require(key.at("keytype") == "ed25519" && key.at("scheme") == "ed25519", ErrorCode::unsupported,
            "This profile supports Ed25519 keys only");
    auto bytes = Unhex(key.at("keyval").at("public").get<std::string>());
    auto sig = Unhex(signature);
    Require(bytes.size() == 32 && sig.size() == 64, ErrorCode::trust, "Invalid Ed25519 encoding");
    Key publicKey(EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, nullptr,
                                              reinterpret_cast<const unsigned char *>(bytes.data()),
                                              bytes.size()),
                  EVP_PKEY_free);
    Digest context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    Require(publicKey && context &&
                EVP_DigestVerifyInit(context.get(), nullptr, nullptr, nullptr, publicKey.get()) == 1,
            ErrorCode::trust, "Signature initialization failed");
    return EVP_DigestVerify(context.get(), reinterpret_cast<const unsigned char *>(sig.data()), sig.size(),
                            reinterpret_cast<const unsigned char *>(message.data()), message.size()) == 1;
}

Json GenerateEd25519Key(const fs::path &path)
{
    Require(!fs::exists(path), ErrorCode::invalidInput, "Refusing to overwrite a signing key");
    Key key(EVP_PKEY_Q_keygen(nullptr, nullptr, "ED25519"), EVP_PKEY_free);
    Require(key != nullptr, ErrorCode::trust, "Key generation failed");
    BIO *bio = BIO_new(BIO_s_mem());
    Require(bio && PEM_write_bio_PrivateKey(bio, key.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1,
            ErrorCode::trust, "Key encoding failed");
    char *data = nullptr;
    auto length = BIO_get_mem_data(bio, &data);
    std::string pem(data, static_cast<std::size_t>(length));
    BIO_free(bio);
    AtomicWrite(path, pem);
    return PublicKey(key.get());
}

Json Sign(const Json &signedValue, const std::vector<fs::path> &paths)
{
    auto message = CanonicalJson(signedValue);
    Json signatures = Json::array();
    OpenSslCrypto crypto;
    for (const auto &path : paths)
    {
        auto key = PrivateKey(path);
        Digest context(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        Require(context && EVP_DigestSignInit(context.get(), nullptr, nullptr, nullptr, key.get()) == 1,
                ErrorCode::trust, "Signing initialization failed");
        unsigned char signature[64];
        std::size_t length = sizeof(signature);
        Require(EVP_DigestSign(context.get(), signature, &length,
                               reinterpret_cast<const unsigned char *>(message.data()), message.size()) == 1,
                ErrorCode::trust, "Signing failed");
        signatures.push_back(
            {{"keyid", crypto.Hash(CanonicalJson(PublicKey(key.get())))}, {"sig", Hex(signature, length)}});
    }
    return {{"signed", signedValue}, {"signatures", signatures}};
}
} // namespace baz
