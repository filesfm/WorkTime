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

find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(worktime_tests
    ${CMAKE_SOURCE_DIR}/test/main.cpp
    ${CMAKE_SOURCE_DIR}/src/core/settings.cpp
    ${CMAKE_SOURCE_DIR}/test/core/settings_test.cpp
)

target_compile_features(worktime_tests PRIVATE cxx_std_17)

target_include_directories(worktime_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(worktime_tests
    PRIVATE
        GTest::gtest
        Qt6::Core
        Qt6::Test
)

include(GoogleTest)
gtest_discover_tests(worktime_tests)
