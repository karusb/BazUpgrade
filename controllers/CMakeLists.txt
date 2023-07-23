set(ELZIP_DECOMPRESS_ONLY ON)

FetchContent_Declare(
  elzip
  GIT_REPOSITORY https://github.com/Sygmei/11Zip.git
)
FetchContent_MakeAvailable(elzip)

add_library(bazupgrade.controllers STATIC)
set_target_properties(bazupgrade.controllers PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.controllers PUBLIC ${BazUpgradePath} elzip)
target_link_libraries(bazupgrade.controllers PUBLIC
    bazupgrade.interfaces
    bazupgrade.util
    elzip
)
target_sources(bazupgrade.controllers PRIVATE
    AssetSelectorContainsString.cpp
    AssetSelectorContainsString.hpp
    AutoUpgraderUnzip.cpp
    AutoUpgraderUnzip.hpp
    AutoUpgraderLaunch.cpp
    AutoUpgraderLaunch.hpp
    DownloaderSelective.cpp
    DownloaderSelective.hpp
    LauncherStd.cpp
    LauncherStd.hpp
    ReleaseSelectorOrdered.cpp
    ReleaseSelectorOrdered.hpp
    UnzipperElzip.cpp
    UnzipperElzip.hpp
)
include(GoogleTest)
include(CTest)
enable_testing()
add_subdirectory(test)