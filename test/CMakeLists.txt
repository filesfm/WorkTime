include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
# GoogleTest must be built with the same runtime library the rest of the
# project uses, or linking fails on Windows/MSVC.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(Qt6 REQUIRED COMPONENTS Gui Test)

add_executable(worktime_tests
    ${CMAKE_SOURCE_DIR}/test/main.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings.cpp
    ${CMAKE_SOURCE_DIR}/src/core/utilities.cpp
    ${CMAKE_SOURCE_DIR}/test/core/settings_test.cpp
    ${CMAKE_SOURCE_DIR}/test/core/utilities_test.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
elseif(APPLE)
    target_sources(worktime_tests PRIVATE ${CMAKE_SOURCE_DIR}/src/core/utilities_mac.mm)
endif()

target_compile_features(worktime_tests PRIVATE cxx_std_17)

target_include_directories(worktime_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(worktime_tests
    PRIVATE
        GTest::gtest
        Qt6::Core
        Qt6::Gui
        Qt6::Test
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(worktime_tests PRIVATE X11::X11)
elseif(APPLE)
    target_link_libraries(worktime_tests PRIVATE "-framework Cocoa")
endif()

include(GoogleTest)
# PRE_TEST (rather than the default POST_BUILD) defers running the test
# binary for test enumeration until `ctest` itself is invoked. worktime_tests
# constructs a QGuiApplication, which needs a working Qt platform plugin
# (e.g. QT_QPA_PLATFORM=offscreen); that's set for the `ctest` invocation in
# CI but not for the build step, so POST_BUILD discovery fails the build.
gtest_discover_tests(worktime_tests DISCOVERY_MODE PRE_TEST)
