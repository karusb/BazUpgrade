#include <BazPO.hpp>
#include <bazupgrade/BazUpgrade.hpp>
#include <iostream>
int main(int argc, const char *argv[])
{
    BazPO::Cli cli(argc, argv, "Installed BazUpgrade C++ SDK consumer");
    cli.strictParsing();
    cli.flag("--capabilities", "Print built-in update capabilities");
    cli.parse();
    std::cout << baz::Capabilities().dump() << '\n';
    return 0;
}