add_library(bazupgrade.interfaces INTERFACE)
set_target_properties(bazupgrade.interfaces PROPERTIES LINKER_LANGUAGE CXX CXX_STANDARD 17)
target_sources(bazupgrade.interfaces PRIVATE
    ApiClient.hpp
    Asset.hpp
    AssetSelector.hpp
    AutoUpgrader.hpp
    Downloader.hpp
    HttpClient.hpp
    Release.hpp
    ReleaseSelector.hpp
    Launcher.hpp
    Unzipper.hpp
)

add_subdirectory(mocks)