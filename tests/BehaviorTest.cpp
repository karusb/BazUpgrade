#include <BazPO.hpp>
#include <gtest/gtest.h>

int main(int argc, const char *argv[])
{
    BazPO::Cli cli(argc, argv, "BazUpgrade native behavior regressions");

    cli.parse();
    ::testing::InitGoogleTest();

    return RUN_ALL_TESTS();
}
