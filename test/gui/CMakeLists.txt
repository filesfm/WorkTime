# Qt Quick Test: runs the tst_*.qml files in this directory against the real
# Main.qml. The controller is replaced by a test double (qml/MainController.qml),
# so the test executable does not link the C++ controller code.
find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 QuickTest Widgets)

qt_add_executable(worktime_gui_tests
    main.cpp
    testdoubles.hpp
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/src/qml/Main.qml PROPERTIES
    QT_RESOURCE_ALIAS "qml/Main.qml"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon.png PROPERTIES
    QT_RESOURCE_ALIAS "icon.png"
)

qt_add_qml_module(worktime_gui_tests
    URI Worktime
    QML_FILES
        ${CMAKE_SOURCE_DIR}/src/qml/Main.qml
        qml/MainController.qml
    RESOURCES
        ${CMAKE_SOURCE_DIR}/resource/icon.png
)

set_target_properties(worktime_gui_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_compile_features(worktime_gui_tests PRIVATE cxx_std_17)

target_link_libraries(worktime_gui_tests
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickTest
        Qt6::Widgets
)

add_test(NAME worktime_gui_tests
    COMMAND worktime_gui_tests -input ${CMAKE_CURRENT_SOURCE_DIR}
)
set_tests_properties(worktime_gui_tests PROPERTIES
    ENVIRONMENT "QT_QUICK_CONTROLS_STYLE=Material"
)
