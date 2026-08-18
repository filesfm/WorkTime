import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform
import Worktime

ApplicationWindow {
    id: window
    visible: !trayAvailable

    Material.theme: Material.System
    Material.accent: Material.Blue

    MainController {
        id: controller
    }

    // Set from C++
    property bool trayAvailable: false

    // Size to fit whatever content is actually in mainColumn, rather than a
    // guessed fixed size that can clip content as fields are added.
    width: mainColumn.implicitWidth + mainColumn.anchors.margins * 2
    height: mainColumn.implicitHeight + mainColumn.anchors.margins * 2
    title: "Worktime"

    onClosing: function (close) {
        if (trayAvailable) {
            close.accepted = false
            window.hide()
        }
    }

    Platform.SystemTrayIcon {
        id: trayIcon
        visible: trayAvailable
        icon.source: "icon.svg"
        tooltip: "Worktime"

        onActivated: function (reason) {
            if (reason === Platform.SystemTrayIcon.Trigger) {
                window.visible = !window.visible
                if (window.visible) {
                    window.raise()
                    window.requestActivate()
                }
            }
        }

        menu: Platform.Menu {
            Platform.MenuItem {
                text: window.visible ? "Hide" : "Show"
                onTriggered: window.visible = !window.visible
            }
            Platform.MenuItem {
                text: "Quit"
                onTriggered: Qt.callLater(Qt.quit)
            }
        }
    }

    ColumnLayout {
        id: mainColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: controller.running ? "#2ecc71" : "#95a5a6"

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            Label {
                text: controller.running ? "Tracking" : "Stopped"
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: controller.running ? "Stop" : "Start"
                highlighted: true
                onClicked: controller.toggleTracking()
            }
        }

        Frame {
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                Label {
                    text: "General"
                    font.bold: true
                    Material.foreground: Material.accent
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8
                    Layout.fillWidth: true

                    Label { text: "Auto Startup" }
                    CheckBox {
                        checked: controller.autoStartup
                        onToggled: controller.autoStartup = checked
                    }
                }

                Label {
                    text: "Account"
                    font.bold: true
                    Material.foreground: Material.accent
                    Layout.topMargin: 8
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8
                    Layout.fillWidth: true

                    Label { text: "Username" }
                    TextField {
                        text: controller.username
                        onEditingFinished: controller.username = text
                        Layout.fillWidth: true
                    }

                    Label { text: "Password" }
                    TextField {
                        text: controller.password
                        echoMode: TextInput.Password
                        onEditingFinished: controller.password = text
                        Layout.fillWidth: true
                    }
                }

                Label {
                    text: "Tracking"
                    font.bold: true
                    Material.foreground: Material.accent
                    Layout.topMargin: 8
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8
                    Layout.fillWidth: true

                    Label { text: "Post Interval (s)" }
                    SpinBox {
                        from: 1
                        to: 86400
                        value: controller.postInterval
                        onValueModified: controller.postInterval = value
                        Layout.fillWidth: true
                    }
                }

                Label {
                    text: "Screenshots"
                    font.bold: true
                    Material.foreground: Material.accent
                    Layout.topMargin: 8
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8
                    Layout.fillWidth: true

                    Label { text: "Enabled" }
                    CheckBox {
                        checked: controller.enableShots
                        onToggled: controller.enableShots = checked
                    }

                    Label { text: "Scale" }
                    Slider {
                        from: 0.01
                        to: 1.0
                        value: controller.shotScale
                        onMoved: controller.shotScale = value
                        Layout.fillWidth: true
                    }

                    Label { text: "Compression" }
                    SpinBox {
                        from: 0
                        to: 100
                        value: controller.shotCompression
                        onValueModified: controller.shotCompression = value
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
