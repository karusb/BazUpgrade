if(NOT TARGET GTest::gtest)
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/063de7e9578f82b369302001269680b4b1553359.zip
        URL_HASH SHA256=f933817755e14daf4afa36230b994c5e9ab5a0476dbd659b1d70163e876b4b91)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_executable(bazupgrade_tests BehaviorTest.cpp TrustTest.cpp EngineTest.cpp TestSupport.cpp)
set_target_properties(bazupgrade_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
target_link_libraries(bazupgrade_tests PRIVATE bazupgrade BazPO::BazPO GTest::gtest)
if(WIN32)
    target_compile_definitions(bazupgrade_tests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_executable(native_installer_fixture InstallerFixture.cpp)
target_link_libraries(native_installer_fixture PRIVATE BazPO::BazPO)

add_test(NAME bazupgrade.behavior COMMAND bazupgrade_tests)
add_test(NAME bazupgrade.cli
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cli_test.py
        $<TARGET_FILE:bazupgrade_cli> $<TARGET_FILE:BazUpgradePublisher>
        $<TARGET_FILE:bazupgrade_sdk> $<TARGET_FILE:native_installer_fixture>)
set_tests_properties(bazupgrade.cli PROPERTIES TIMEOUT 120)
