#pragma once

#include "util/Json.hpp"
#include <atomic>
#include <optional>

namespace baz
{
struct Cancellation
{
    std::atomic<bool> requested{false};
    void Check() const
    {
        Require(!requested.load(), ErrorCode::cancelled, "Operation cancelled");
    }
};

class Transport
{
  public:
    virtual ~Transport() = default;
    virtual std::optional<std::string> Fetch(const std::string &path, std::uint64_t limit,
                                             const Cancellation &cancel) = 0;
    virtual void Download(const std::string &path, const fs::path &destination, std::uint64_t limit,
                          const Cancellation &cancel) = 0;
};
} // namespace baz
