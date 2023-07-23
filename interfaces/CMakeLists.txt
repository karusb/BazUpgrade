add_library(bazupgrade.interfaces INTERFACE)
set_target_properties(bazupgrade.interfaces PROPERTIES LINKER_LANGUAGE CXX)
target_sources(bazupgrade.interfaces PRIVATE
    ApiClient.hpp
    AutoUpgrader.hpp
    Downloader.hpp
    HttpClient.hpp
    Launcher.hpp
    Unzipper.hpp
)

add_subdirectory(mocks)