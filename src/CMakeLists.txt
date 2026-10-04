qt_add_executable(worktime WIN32 MACOSX_BUNDLE
    main.cpp
)

set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon.svg PROPERTIES
    QT_RESOURCE_ALIAS "icon.svg"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon.png PROPERTIES
    QT_RESOURCE_ALIAS "icon.png"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon-green.png PROPERTIES
    QT_RESOURCE_ALIAS "icon-green.png"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/icon-red.png PROPERTIES
    QT_RESOURCE_ALIAS "icon-red.png"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/kwin/focused-window-title.js PROPERTIES
    QT_RESOURCE_ALIAS "focused-window-title.js"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/kwin/popup-placement.js PROPERTIES
    QT_RESOURCE_ALIAS "popup-placement.js"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/gnome/worktime-placement/metadata.json PROPERTIES
    QT_RESOURCE_ALIAS "worktime-placement-metadata.json"
)
set_source_files_properties(${CMAKE_SOURCE_DIR}/resource/gnome/worktime-placement/extension.js PROPERTIES
    QT_RESOURCE_ALIAS "worktime-placement-extension.js"
)

qt_add_qml_module(worktime
    URI Worktime
    QML_FILES
        qml/Main.qml
    SOURCES
        qml/controller/maincontroller.cpp
        qml/controller/maincontroller.hpp
        qml/controller/appquitter.cpp
        qml/controller/appquitter.hpp
        qml/controller/screengeometry.cpp
        qml/controller/screengeometry.hpp
        qml/controller/popupplacement.cpp
        qml/controller/popupplacement.hpp
        core/settings/settings.cpp
        core/settings/settings.hpp
        core/settings/credentialstore.cpp
        core/settings/credentialstore.hpp
        core/network/sendingservice.cpp
        core/network/sendingservice.hpp
        core/network/notsentsender.cpp
        core/network/notsentsender.hpp
        core/network/shootrequestbuilder.cpp
        core/network/shootrequestbuilder.hpp
        core/activity/activitymonitor.cpp
        core/activity/activitymonitor.hpp
        core/utilities.cpp
        core/utilities.hpp
        core/oneinstanceguarantor.cpp
        core/oneinstanceguarantor.hpp
        core/kdefocusedwindowtitle.cpp
        core/kdefocusedwindowtitle.hpp
        core/kwinscript.cpp
        core/kwinscript.hpp
        core/storage/sqliteconnection.hpp
        core/storage/sqliteconnection.cpp
    RESOURCES
        ${CMAKE_SOURCE_DIR}/resource/icon.svg
        ${CMAKE_SOURCE_DIR}/resource/icon.png
        ${CMAKE_SOURCE_DIR}/resource/icon-green.png
        ${CMAKE_SOURCE_DIR}/resource/icon-red.png
        ${CMAKE_SOURCE_DIR}/resource/kwin/focused-window-title.js
        ${CMAKE_SOURCE_DIR}/resource/kwin/popup-placement.js
        ${CMAKE_SOURCE_DIR}/resource/gnome/worktime-placement/metadata.json
        ${CMAKE_SOURCE_DIR}/resource/gnome/worktime-placement/extension.js
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(UDEV REQUIRED IMPORTED_TARGET libudev)
    find_package(KF6IdleTime REQUIRED)
elseif(WIN32)
    target_sources(worktime PRIVATE ${CMAKE_SOURCE_DIR}/resource/windows/icon.rc)
elseif(APPLE)
    target_sources(worktime PRIVATE core/utilities_mac.mm core/activity/activitymonitor_mac.mm)

    set(ICON_FILE ${CMAKE_SOURCE_DIR}/resource/macos/icon.icns)
    set_source_files_properties(${ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
    target_sources(worktime PRIVATE ${ICON_FILE})

    set(WORKTIME_APP_BUNDLE_DIR "${CMAKE_BINARY_DIR}/bin/WorkTime.app")
    add_custom_command(TARGET worktime POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E rename "$<TARGET_BUNDLE_DIR:worktime>" "${WORKTIME_APP_BUNDLE_DIR}"
        COMMENT "Renaming app bundle to WorkTime.app"
    )
endif()

set_target_properties(worktime PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    EXPORT_COMPILE_COMMANDS YES
    MACOSX_BUNDLE_ICON_FILE icon.icns
    MACOSX_BUNDLE_BUNDLE_NAME WorkTime
    MACOSX_BUNDLE_COPYRIGHT "Copyright 2026 WorkTime contributors. Licensed under Apache-2.0."
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resource/macos/Info.plist.in
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.filesfm.worktime
)

target_compile_features(worktime PRIVATE cxx_std_17)

if(BUILD_WITHOUT_AUTOSTART)
    target_compile_definitions(worktime PRIVATE BUILD_WITHOUT_AUTOSTART)
endif()

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
        Qt6::DBus
        Qt6::Sql
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(worktime
        PRIVATE
            X11::X11
            PkgConfig::UDEV
            KF6::IdleTime
    )
elseif(APPLE)
    target_link_libraries(worktime
        PRIVATE
            "-framework Cocoa"
            "-framework ApplicationServices"
            "-framework ServiceManagement"
            "-framework CoreFoundation"
            "-framework AppKit"
            "-framework UserNotifications"
            "-framework Security"
    )
elseif(WIN32)
    target_link_libraries(worktime
        PRIVATE
            User32
            Advapi32
    )
endif()

include(GNUInstallDirs)
install(TARGETS worktime
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .
)

find_package(Doxygen)
if(BUILD_DOCS)
    if(Doxygen_FOUND)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)

        doxygen_add_docs(docs ALL USE_STAMP_FILE
            qml/controller/maincontroller.cpp
            qml/controller/maincontroller.hpp
            qml/controller/appquitter.cpp
            qml/controller/appquitter.hpp
            qml/controller/screengeometry.cpp
            qml/controller/screengeometry.hpp
            qml/controller/popupplacement.cpp
            qml/controller/popupplacement.hpp
            core/settings/settings.cpp
            core/settings/settings.hpp
            core/network/sendingservice.cpp
            core/network/sendingservice.hpp
            core/activity/activitymonitor.cpp
            core/activity/activitymonitor.hpp
            core/utilities.cpp
            core/utilities.hpp
            core/oneinstanceguarantor.cpp
            core/oneinstanceguarantor.hpp
            core/kdefocusedwindowtitle.cpp
            core/kdefocusedwindowtitle.hpp
            core/kwinscript.cpp
            core/kwinscript.hpp
            core/storage/sqliteconnection.hpp
            core/storage/sqliteconnection.cpp
            COMMENT "Generating API documentation with Doxygen"
        )
    else()
        message(STATUS "Doxygen not found - 'docs' target will not be available")
    endif()
endif()
