#pragma once

#include "interfaces/Transport.hpp"
#include <memory>

class HTTPClientCurl;

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
    std::string ResolveUrl(const std::string &path) const;
    std::string baseUrl;
    std::unique_ptr<HTTPClientCurl> client;
};
} // namespace baz
