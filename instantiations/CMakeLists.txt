add_library(bazupgrade.instantiations OBJECT)
set_target_properties(bazupgrade.instantiations PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.instantiations PUBLIC ${BazUpgradePath} elzip)
add_dependencies(bazupgrade.instantiations bazupgrade.curl)
target_link_libraries(bazupgrade.instantiations PUBLIC
    bazupgrade.curl
    bazupgrade.controllers
    bazupgrade.vendors.github
)
target_sources(bazupgrade.instantiations PRIVATE
    BazUpgradeGitHub.hpp
)

add_library(bazupgrade.instantiations_tracing OBJECT)
set_target_properties(bazupgrade.instantiations_tracing PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(bazupgrade.instantiations_tracing PUBLIC ${BazUpgradePath} elzip)
add_dependencies(bazupgrade.instantiations_tracing bazupgrade.curl)
target_link_libraries(bazupgrade.instantiations_tracing PUBLIC
    bazupgrade.curl
    bazupgrade.controllers
    bazupgrade.vendors.github
    bazupgrade.tracing
)
target_sources(bazupgrade.instantiations_tracing PRIVATE
    TracingBazUpgradeGitHub.hpp
)