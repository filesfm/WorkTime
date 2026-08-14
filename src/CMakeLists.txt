qt_add_executable(worktime WIN32 MACOSX_BUNDLE
    main.cpp
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon.svg PROPERTIES
    QT_RESOURCE_ALIAS "icon.svg"
)

qt_add_qml_module(worktime
    URI Worktime
    QML_FILES
        qml/Main.qml
    SOURCES
        qml/controller/maincontroller.cpp
        qml/controller/maincontroller.hpp
        core/settings.cpp
        core/settings.hpp
        core/sendingservice.cpp
        core/sendingservice.hpp
    RESOURCES
        ${CMAKE_SOURCE_DIR}/resource/icon.svg
)

set_target_properties(worktime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_features(worktime PRIVATE cxx_std_17)

target_include_directories(worktime
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/qml/controller
)

target_link_libraries(worktime
    PRIVATE
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Widgets
)
