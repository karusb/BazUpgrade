include(GoogleTest)
include(CTest)
enable_testing()

add_executable(
  bazupgrade.controllers_test
  AssetSelectorContainsStringTest.cpp
  AutoUpgraderUnzipTest.cpp
  AutoUpgraderLaunchTest.cpp
  DownloaderSelectiveTest.cpp
  ReleaseSelectorOrderedTest.cpp
)

target_include_directories(bazupgrade.controllers_test PUBLIC ${BazUpgradePath} ${googletest_SOURCE_DIR})
set_target_properties(bazupgrade.controllers_test PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 17)
target_link_libraries(
  bazupgrade.controllers_test
  bazupgrade.controllers
  bazupgrade.interfaces.mocks
  gtest
  gtest_main
)
gtest_discover_tests(bazupgrade.controllers_test)