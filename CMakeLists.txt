cmake_minimum_required(VERSION 3.19)

project(worktime VERSION 0.1.0 LANGUAGES CXX)

if(APPLE)
    enable_language(OBJCXX)
endif()

option(BUILD_DOCS "Build the API documentation with Doxygen" OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network Qml Quick QuickControls2 Widgets)

qt_standard_project_setup(REQUIRES 6.5 SUPPORTS_UP_TO 6.8)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

find_package(Doxygen)
if(BUILD_DOCS)
    if(Doxygen_FOUND)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

        doxygen_add_docs(docs ALL USE_STAMP_FILE
            ${CMAKE_SOURCE_DIR}/src/core/settings.hpp
            ${CMAKE_SOURCE_DIR}/src/core/sendingservice.hpp
            ${CMAKE_SOURCE_DIR}/src/core/utilities.hpp
            ${CMAKE_SOURCE_DIR}/src/qml/controller/maincontroller.hpp
            COMMENT "Generating API documentation with Doxygen"
        )
    else()
        message(STATUS "Doxygen not found - 'docs' target will not be available")
    endif()
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
