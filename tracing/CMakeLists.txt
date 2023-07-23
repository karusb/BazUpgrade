add_library(bazupgrade.tracing STATIC)
set_target_properties(bazupgrade.tracing PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.tracing PUBLIC ${BazUpgradePath} elzip)
target_link_libraries(bazupgrade.tracing PUBLIC
    bazupgrade.interfaces
    bazupgrade.util
)
target_sources(bazupgrade.tracing PRIVATE
    TracingApiClient.cpp
    TracingApiClient.hpp
    TracingAssetSelector.cpp
    TracingAssetSelector.hpp
    TracingReleaseSelector.cpp
    TracingReleaseSelector.hpp
)