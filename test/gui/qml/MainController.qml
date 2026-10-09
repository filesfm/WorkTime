import QtQuick

// Test double for the C++ MainController (src/qml/controller/maincontroller.hpp).
// The GUI test executable does not compile the real controller, so this file
// provides the MainController type to Main.qml. That keeps the tests away from
// QSettings, the OS credential store, the network and the sqlite database.
QtObject {
    property string username: ""
    property string password: ""
    property bool autoStartup: false
    property bool startButtonPushed: false
    property real sentCountLast24Hours: 0
    property real sentBytesLast24Hours: 0
    property url statusIconSource: "qrc:/qt/qml/Worktime/icon.png"
    property bool credentialsRejected: false

    // Number of times Main.qml asked to start or stop tracking.
    property int toggleCount: 0

    function toggleTracking() {
        toggleCount += 1
        startButtonPushed = !startButtonPushed
    }
}
