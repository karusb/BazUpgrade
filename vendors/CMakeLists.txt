add_library(bazupgrade.vendors.github STATIC)
set_target_properties(bazupgrade.vendors.github PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.vendors.github PUBLIC ${BazUpgradePath} elzip)
target_link_libraries(bazupgrade.vendors.github PUBLIC
    bazupgrade.interfaces
    bazupgrade.util
)
target_sources(bazupgrade.vendors.github PRIVATE
    ApiClientGitHub.cpp
    ApiClientGitHub.hpp
    ResponseParserGitHub.cpp
    ResponseParserGitHub.hpp
)
include(GoogleTest)
include(CTest)
enable_testing()
add_subdirectory(test)