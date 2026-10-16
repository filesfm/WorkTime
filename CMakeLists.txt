cmake_minimum_required(VERSION 3.19)

project(worktime VERSION 0.6.5 LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
elseif(WIN32)
    enable_language(RC)
endif()

option(BUILD_DOCS "Build the API documentation with Doxygen" OFF)
option(BUILD_WITHOUT_AUTOSTART "Exclude the autostart feature, for example, for AppImage builds" OFF)
option(BUILD_WITHOUT_KF6IDLETIME "Exclude the KDE Plasma KIdleTime backend, for example, where libkf6idletime-dev isn't packaged" OFF)
option(ENABLE_COVERAGE "Instrument worktime_tests for coverage (GCC/Clang only)" OFF)

# Compiled into the worktime binary (see src/core/logging/betterstackconfig.hpp.in);
# there is no runtime/user-facing setting for these. Left empty, critical/fatal
# log forwarding to Better Stack Logs is simply disabled.
set(BETTERSTACK_SOURCE_TOKEN "$ENV{BETTERSTACK_SOURCE_TOKEN}" CACHE STRING
    "Better Stack Logs source token used to forward critical/fatal log messages; empty disables forwarding")
set(BETTERSTACK_INGESTING_HOST "$ENV{BETTERSTACK_INGESTING_HOST}" CACHE STRING
    "Better Stack Logs ingesting host (e.g. in.logs.betterstack.com) paired with BETTERSTACK_SOURCE_TOKEN")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Qml Quick QuickControls2 Widgets DBus Sql)

qt_standard_project_setup(REQUIRES 6.5 SUPPORTS_UP_TO 6.8)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/.git)
    execute_process(
        COMMAND git config core.hooksPath hooks
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE _hooks_result
    )
    if(NOT _hooks_result EQUAL 0)
        message(WARNING "Failed to set core.hooksPath")
    endif()
endif()
