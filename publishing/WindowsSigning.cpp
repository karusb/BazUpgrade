#include "publishing/WindowsSigning.hpp"
#include "crypto/OpenSslCrypto.hpp"
#include "util/WindowsProcess.hpp"
#include <windows.h>

#include <algorithm>
#include <bcrypt.h>
#include <ncrypt.h>
#include <objbase.h>
#include <wincrypt.h>

namespace baz
{
namespace
{
struct Provider
{
    NCRYPT_PROV_HANDLE value = 0;
    ~Provider()
    {
        if (value)
            NCryptFreeObject(value);
    }
};
struct Key
{
    NCRYPT_KEY_HANDLE value = 0;
    bool persist = false;
    ~Key()
    {
        if (!value)
            return;
        if (persist)
            NCryptFreeObject(value);
        else
            NCryptDeleteKey(value, 0);
    }
};
struct Certificate
{
    PCCERT_CONTEXT value = nullptr;
    ~Certificate()
    {
        if (value)
            CertFreeCertificateContext(value);
    }
};
struct Store
{
    HCERTSTORE value = nullptr;
    ~Store()
    {
        if (value)
            CertCloseStore(value, 0);
    }
};
std::vector<BYTE> Encode(LPCSTR type, const void *value)
{
    DWORD size = 0;
    Require(CryptEncodeObjectEx(X509_ASN_ENCODING, type, value, 0, nullptr, nullptr, &size), ErrorCode::trust,
            "Cannot size certificate extension");
    std::vector<BYTE> bytes(size);
    Require(CryptEncodeObjectEx(X509_ASN_ENCODING, type, value, 0, nullptr, bytes.data(), &size),
            ErrorCode::trust, "Cannot encode certificate extension");
    return bytes;
}
} // namespace
Json CreateTestCertificate(const fs::path &output)
{
    Require(!fs::exists(output) && !fs::is_symlink(fs::symlink_status(output)), ErrorCode::invalidInput,
            "Certificate output already exists");
    GUID id{};
    Require(CoCreateGuid(&id) == S_OK, ErrorCode::io, "Cannot allocate test key identifier");
    wchar_t identifier[40]{};
    Require(StringFromGUID2(id, identifier, 40) > 0, ErrorCode::io, "Cannot encode test key identifier");
    std::wstring keyName = L"BazUpgrade-Test-" + std::wstring(identifier);
    Provider provider;
    Key key;
    Require(NCryptOpenStorageProvider(&provider.value, MS_KEY_STORAGE_PROVIDER, 0) == ERROR_SUCCESS,
            ErrorCode::trust, "Cannot open Windows key provider");
    Require(NCryptCreatePersistedKey(provider.value, &key.value, NCRYPT_RSA_ALGORITHM, keyName.c_str(), 0,
                                     0) == ERROR_SUCCESS,
            ErrorCode::trust, "Cannot create test signing key");
    DWORD bits = 3072;
    Require(NCryptSetProperty(key.value, NCRYPT_LENGTH_PROPERTY, reinterpret_cast<PBYTE>(&bits), sizeof(bits),
                              0) == ERROR_SUCCESS &&
                NCryptFinalizeKey(key.value, 0) == ERROR_SUCCESS,
            ErrorCode::trust, "Cannot finalize test signing key");

    const wchar_t *subject = L"CN=BazUpgrade Test";
    DWORD subjectSize = 0;
    Require(CertStrToNameW(X509_ASN_ENCODING, subject, CERT_X500_NAME_STR, nullptr, nullptr, &subjectSize,
                           nullptr),
            ErrorCode::trust, "Cannot size test certificate subject");
    std::vector<BYTE> subjectBytes(subjectSize);
    Require(CertStrToNameW(X509_ASN_ENCODING, subject, CERT_X500_NAME_STR, nullptr, subjectBytes.data(),
                           &subjectSize, nullptr),
            ErrorCode::trust, "Cannot encode test certificate subject");
    CERT_NAME_BLOB name{subjectSize, subjectBytes.data()};
    LPSTR usages[] = {const_cast<LPSTR>(szOID_PKIX_KP_CODE_SIGNING)};
    CERT_ENHKEY_USAGE usage{1, usages};
    auto eku = Encode(X509_ENHANCED_KEY_USAGE, &usage);
    BYTE signingUsage = CERT_DIGITAL_SIGNATURE_KEY_USAGE;
    CRYPT_BIT_BLOB keyUsage{1, &signingUsage, 0};
    auto encodedUsage = Encode(X509_KEY_USAGE, &keyUsage);
    CERT_BASIC_CONSTRAINTS2_INFO basic{FALSE, FALSE, 0};
    auto encodedBasic = Encode(X509_BASIC_CONSTRAINTS2, &basic);
    CERT_EXTENSION values[] = {
        {const_cast<LPSTR>(szOID_ENHANCED_KEY_USAGE), FALSE, {static_cast<DWORD>(eku.size()), eku.data()}},
        {const_cast<LPSTR>(szOID_KEY_USAGE),
         TRUE,
         {static_cast<DWORD>(encodedUsage.size()), encodedUsage.data()}},
        {const_cast<LPSTR>(szOID_BASIC_CONSTRAINTS2),
         TRUE,
         {static_cast<DWORD>(encodedBasic.size()), encodedBasic.data()}}};
    CERT_EXTENSIONS extensions{3, values};
    CRYPT_KEY_PROV_INFO providerInfo{};
    providerInfo.pwszContainerName = keyName.data();
    providerInfo.pwszProvName = const_cast<LPWSTR>(MS_KEY_STORAGE_PROVIDER);
    providerInfo.dwProvType = 0; // CNG provider; legacy key spec is zero.
    CRYPT_ALGORITHM_IDENTIFIER algorithm{const_cast<LPSTR>(szOID_RSA_SHA256RSA), {0, nullptr}};
    FILETIME now{};
    GetSystemTimeAsFileTime(&now);
    ULARGE_INTEGER time{};
    time.LowPart = now.dwLowDateTime;
    time.HighPart = now.dwHighDateTime;
    ULARGE_INTEGER start{}, end{};
    start.QuadPart = time.QuadPart - 300ULL * 10000000;
    end.QuadPart = time.QuadPart + 7ULL * 86400 * 10000000;
    FILETIME startFile{start.LowPart, start.HighPart}, endFile{end.LowPart, end.HighPart};
    SYSTEMTIME starts{}, ends{};
    Require(FileTimeToSystemTime(&startFile, &starts) && FileTimeToSystemTime(&endFile, &ends), ErrorCode::io,
            "Cannot encode certificate validity");
    Certificate certificate;
    certificate.value = CertCreateSelfSignCertificate(key.value, &name, 0, &providerInfo, &algorithm, &starts,
                                                      &ends, &extensions);
    Require(certificate.value != nullptr, ErrorCode::trust, "Cannot create code-signing certificate");
    Store store;
    store.value = CertOpenStore(CERT_STORE_PROV_SYSTEM_W, 0, 0,
                                CERT_SYSTEM_STORE_CURRENT_USER | CERT_STORE_OPEN_EXISTING_FLAG, L"MY");
    Require(store.value != nullptr, ErrorCode::trust,
            "Cannot open explicitly selected user certificate store");
    PCCERT_CONTEXT stored = nullptr;
    Require(CertAddCertificateContextToStore(store.value, certificate.value, CERT_STORE_ADD_NEW, &stored),
            ErrorCode::trust, "Cannot store test certificate");
    try
    {
        BYTE thumbprint[20];
        DWORD size = sizeof(thumbprint);
        Require(CertGetCertificateContextProperty(stored, CERT_SHA1_HASH_PROP_ID, thumbprint, &size),
                ErrorCode::trust, "Cannot identify test certificate");
        Json result = {{"status", "created"},
                       {"testOnly", true},
                       {"subject", "CN=BazUpgrade Test"},
                       {"thumbprint", Hex(thumbprint, size)},
                       {"store", "CurrentUser/My"},
                       {"validityDays", 7},
                       {"publicCertificate", fs::absolute(output).generic_u8string()}};
        AtomicWrite(output, std::string(reinterpret_cast<const char *>(certificate.value->pbCertEncoded),
                                        certificate.value->cbCertEncoded));
        key.persist = true;
        CertFreeCertificateContext(stored);
        stored = nullptr;
        return result;
    }
    catch (...)
    {
        if (stored)
            CertDeleteCertificateFromStore(stored);
        throw;
    }
}
Json SignWindowsArtifact(const fs::path &artifact, const std::string &thumbprint, const fs::path &signingTool,
                         const std::string &timestampUrl)
{
    Require(fs::is_regular_file(artifact) && !fs::is_symlink(artifact), ErrorCode::invalidInput,
            "Signing requires a completed regular artifact");
    Require(signingTool.is_absolute() && fs::is_regular_file(signingTool) && !fs::is_symlink(signingTool),
            ErrorCode::invalidInput, "Select an absolute trusted Windows SDK signtool executable");
    Require(thumbprint.size() == 40 &&
                std::all_of(
                    thumbprint.begin(), thumbprint.end(), [](char c)
                    { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }),
            ErrorCode::invalidInput,
            "Select a SHA-1 certificate-store thumbprint (signature digest is SHA-256)");
    Require(timestampUrl.empty() || (timestampUrl.rfind("https://", 0) == 0 &&
                                     timestampUrl.find_first_of("@\r\n") == std::string::npos),
            ErrorCode::invalidInput, "Timestamp service must be an explicitly selected HTTPS endpoint");
    std::vector<std::wstring> arguments{L"sign", L"/fd", L"SHA256", L"/sha1",
                                        fs::u8path(thumbprint).wstring()};
    if (!timestampUrl.empty())
    {
        arguments.insert(arguments.end(), {L"/tr", fs::u8path(timestampUrl).wstring(), L"/td", L"SHA256"});
    }
    arguments.push_back(fs::absolute(artifact).wstring());
    auto signLog = artifact;
    signLog += ".sign.log";
    Require(RunWindowsProcess(signingTool.wstring(), arguments, signLog) == 0, ErrorCode::trust,
            "Windows artifact signing failed; inspect .sign.log");
    auto verifyLog = artifact;
    verifyLog += ".verify.log";
    Require(RunWindowsProcess(signingTool.wstring(), {L"verify", L"/pa", fs::absolute(artifact).wstring()},
                              verifyLog) == 0,
            ErrorCode::trust, "Signed artifact did not pass Windows trust verification; inspect .verify.log");
    return {{"status", "signed"},
            {"artifact", fs::absolute(artifact).generic_u8string()},
            {"digest", "sha256"},
            {"certificateThumbprint", thumbprint}};
}
} // namespace baz
