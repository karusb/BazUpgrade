#pragma once

#include "interfaces/Transport.hpp"
#include <iosfwd>
#include <memory>

class HTTPClientCurl;
namespace curlwrapper
{
class HttpUrl;
class Error;
struct RequestOptions;
struct TransferOptions;
} // namespace curlwrapper

namespace baz
{
class DirectoryTransport final : public Transport
{
  public:
    explicit DirectoryTransport(fs::path root);
    std::optional<std::string> Fetch(const std::string &path, std::uint64_t limit,
                                     const Cancellation &cancel) override;
    void Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                  const Cancellation &cancel) override;

  private:
    static void CopyArtifact(std::istream &input, std::ostream &output, std::uint64_t limit,
                             const Cancellation &cancel);
    fs::path Resolve(const std::string &path) const;
    fs::path root;
};

class CurlTransport final : public Transport
{
  public:
    explicit CurlTransport(std::string baseUrl, bool allowLoopbackHttp = false);
    ~CurlTransport() override;
    std::optional<std::string> Fetch(const std::string &path, std::uint64_t limit,
                                     const Cancellation &cancel) override;
    void Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                  const Cancellation &cancel) override;

  private:
    static void ValidateRepositoryUrl(const curlwrapper::HttpUrl &url, bool allowLoopbackHttp);
    static curlwrapper::RequestOptions MakeClientOptions(bool allowLoopbackHttp);
    static curlwrapper::TransferOptions MakeTransferOptions(std::uint64_t limit, const Cancellation &cancel);
    static Error TranslateError(const curlwrapper::Error &error, ErrorCode inputError = ErrorCode::transport);
    std::string ResolveUrl(const std::string &path) const;
    std::unique_ptr<curlwrapper::HttpUrl> baseUrl;
    std::unique_ptr<HTTPClientCurl> client;
};
} // namespace baz
