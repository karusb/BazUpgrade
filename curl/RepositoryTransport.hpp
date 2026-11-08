#pragma once

#include "interfaces/Transport.hpp"

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
    std::optional<std::string> Fetch(const std::string &path, std::uint64_t limit,
                                     const Cancellation &cancel) override;
    void Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                  const Cancellation &cancel) override;

  private:
    std::string baseUrl;
    bool loopbackHttp;
};
} // namespace baz
