add_library(bazupgrade.interfaces.mocks INTERFACE)
set_target_properties(bazupgrade.interfaces.mocks PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 17)
target_include_directories(bazupgrade.interfaces.mocks INTERFACE ${BazUpgradePath} ${googletest_SOURCE_DIR})
target_link_libraries(bazupgrade.interfaces.mocks INTERFACE
    bazupgrade.interfaces
    gmock
)
target_sources(bazupgrade.interfaces.mocks PRIVATE
    ApiClientMock.hpp
    AssetSelectorMock.hpp
    AutoUpgraderMock.hpp
    DownloaderMock.hpp
    HttpClientMock.hpp
    LauncherMock.hpp
    ReleaseSelectorMock.hpp
    ResponseParserMock.hpp
    UnzipperMock.hpp
)
