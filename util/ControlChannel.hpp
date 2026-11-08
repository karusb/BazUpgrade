#pragma once
#include "interfaces/Transport.hpp"
#include <thread>
namespace baz
{
// Cooperative stdin control avoids Windows TerminateProcess-style signal cancellation.
class ControlChannel
{
  public:
    ControlChannel(Cancellation &cancellation, bool enabled);
    ~ControlChannel();
    ControlChannel(const ControlChannel &) = delete;
    ControlChannel &operator=(const ControlChannel &) = delete;

  private:
    std::atomic<bool> stopping{false};
    std::thread reader;
};
} // namespace baz
