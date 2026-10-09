#include "testdoubles.hpp"

#include <QApplication>

#include <QtQuickTest/quicktest.h>

// QApplication rather than QGuiApplication, like the app itself: Qt Labs
// Platform's tray icon and menu (used by Main.qml) need Qt Widgets.
// Runs every tst_*.qml file in the directory passed with -input (see
// add_test() in test/gui/CMakeLists.txt). quick_test_main_with_setup(),
// rather than quick_test_main(), installs the QuickTestSetup doubles (see
// testdoubles.hpp) before any QML test file loads.
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QuickTestSetup setup;
    return quick_test_main_with_setup(argc, argv, "worktime_gui", nullptr, &setup);
}
