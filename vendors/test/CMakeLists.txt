include(GoogleTest)
include(CTest)
enable_testing()

add_executable(
  bazupgrade.vendors.github_test
  ApiClientGitHubTest.cpp
  ResponseParserGitHubTest.cpp
)

target_include_directories(bazupgrade.vendors.github_test PUBLIC ${BazUpgradePath} ${googletest_SOURCE_DIR})
set_property(TARGET bazupgrade.vendors.github_test PROPERTY CXX_STANDARD 17)
target_link_libraries(
  bazupgrade.vendors.github_test
  bazupgrade.vendors.github
  bazupgrade.interfaces.mocks
  gtest
  gtest_main
)
gtest_discover_tests(bazupgrade.vendors.github_test)