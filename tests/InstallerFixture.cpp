#include <BazPO.hpp>
#include <iostream>
int main(int argc, const char *argv[])
{
    BazPO::Cli cli(argc, argv, "Synthetic NSIS process contract; never installs a package");
    cli.strictParsing();
    cli.flag("/S", "Synthetic silent-install contract").mandatory();
    cli.parse();
    std::cout << "synthetic installer progress\n";
    return 0;
}