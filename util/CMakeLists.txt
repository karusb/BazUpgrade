add_library(bazupgrade.util STATIC)
set_target_properties(bazupgrade.util PROPERTIES LINKER_LANGUAGE CXX)

target_sources(bazupgrade.util PRIVATE
    FILEWrapper.cpp
    FILEWrapper.hpp
    JsonHelper.cpp
    JsonHelper.hpp
)