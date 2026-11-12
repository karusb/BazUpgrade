#pragma once

#include "interfaces/Transport.hpp"
#include <iosfwd>
#include <memory>

class HTTPClient;
namespace curlwrapper
{
struct Response;
}

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
    CurlTransport(std::string baseUrl, std::unique_ptr<HTTPClient> client, bool allowLoopbackHttp = false);
    ~CurlTransport() override;
    std::optional<std::string> Fetch(const std::string &path, std::uint64_t limit,
                                     const Cancellation &cancel) override;
    void Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                  const Cancellation &cancel) override;

  private:
    static std::string EncodePath(const std::string &path);
    curlwrapper::Response Perform(const std::string &path, std::uint64_t limit, const Cancellation &cancel,
                                  std::ofstream *file);
    std::string baseUrl;
    bool loopbackHttp;
    std::unique_ptr<HTTPClient> client;
};
} // namespace baz
