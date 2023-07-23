add_library(bazupgrade.interfaces.mocks INTERFACE)
set_target_properties(bazupgrade.interfaces.mocks PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.interfaces.mocks INTERFACE ${BazUpgradePath} ${googletest_SOURCE_DIR})
target_link_libraries(bazupgrade.interfaces.mocks INTERFACE
    bazupgrade.interfaces
    gmock
)
target_sources(bazupgrade.interfaces.mocks PRIVATE
    ApiClientMock.hpp
    AutoUpgraderMock.hpp
    DownloaderMock.hpp
    HttpClientMock.hpp
    LauncherMock.hpp
    ResponseParserMock.hpp
    UnzipperMock.hpp
)
