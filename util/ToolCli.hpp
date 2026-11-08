#pragma once
#include "util/Json.hpp"
#include <BazPO.hpp>
#include <sstream>
namespace baz
{
struct CliExit
{
    std::string output;
};
class ToolCli final : public BazPO::Cli
{
  public:
    ToolCli(int argc, const char **argv, const char *description) : BazPO::Cli(argc, argv, description)
    {
        changeIO(&diagnostics);
    }
    std::string Help()
    {
        diagnostics.str("");
        printOptions();
        return diagnostics.str();
    }

  private:
    void exitWithCode(int code) override
    {
        if (code == 0)
            throw CliExit{Help()};
        throw Error(ErrorCode::invalidInput, diagnostics.str());
    }
    std::ostringstream diagnostics;
};
inline std::string Required(const BazPO::Option &option)
{
    Require(option.exists() && !option.valueAs<std::string>().empty(), ErrorCode::invalidInput,
            "A required option is missing");
    return option.valueAs<std::string>();
}
} // namespace baz
