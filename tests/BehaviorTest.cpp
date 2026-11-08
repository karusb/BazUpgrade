#include "tests/TestSupport.hpp"
#include <BazPO.hpp>
namespace test
{
unsigned passed = 0, failed = 0;
}
int main(int argc, const char *argv[])
{
    BazPO::Cli cli(argc, argv, "BazUpgrade native behavior regressions");
    cli.parse();
    test::TrustTests();
    test::EngineTests();
    std::cout << test::passed << " passed; " << test::failed << " failed\n";
    return test::failed ? 1 : 0;
}
