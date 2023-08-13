# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.12)

project(BazUpgrade VERSION 1.0.0 DESCRIPTION "BazUpgrade")

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

SET(requiredlibs)
SET(DOWNLOAD_EXTRACT_TIMESTAMP ON)

add_executable (BazUpgrade
 "BazUpgrade.cpp" 
)
target_include_directories(BazUpgrade PUBLIC)
set_property(TARGET BazUpgrade PROPERTY CXX_STANDARD 17)

include(ExternalProject)
include(FetchContent)
FetchContent_Declare(
  BazPO
  GIT_REPOSITORY https://github.com/karusb/BazPO.git
  GIT_TAG        v0.2.1
)

FetchContent_MakeAvailable(BazPO)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
set_property(TARGET gtest PROPERTY CXX_STANDARD 17)

include(GoogleTest)
include(CTest)
enable_testing()

set(BazUpgradePath ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(interfaces)
add_subdirectory(curl)
add_subdirectory(util)
add_subdirectory(controllers)
add_subdirectory(vendors)
add_subdirectory(tracing)
add_subdirectory(instantiations)

add_dependencies(BazUpgrade bazupgrade.curl)
target_link_libraries(
  BazUpgrade PUBLIC
  BazPO
  bazupgrade.curl
  bazupgrade.instantiations
  bazupgrade.instantiations_tracing
)
