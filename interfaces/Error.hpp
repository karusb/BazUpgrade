#pragma once

#include <stdexcept>
#include <string>

namespace baz
{
enum class ErrorCode
{
    invalidInput,
    trust,
    expired,
    rollback,
    transport,
    cancelled,
    busy,
    unsupported,
    io,
    unknownOutcome
};

class Error : public std::runtime_error
{
  public:
    Error(ErrorCode code, const std::string &message) : std::runtime_error(message), code(code)
    {
    }
    ErrorCode code;
};

void Require(bool condition, ErrorCode code, const std::string &message);
} // namespace baz
