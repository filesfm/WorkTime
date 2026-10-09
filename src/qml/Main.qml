import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform
import Worktime

ApplicationWindow {
    id: window
    visible: !trayAvailable

    flags: trayAvailable
        ? Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
        : Qt.Window

    Material.theme: Material.System
    Material.accent: Material.Blue

    MainController {
        id: controller
        objectName: "controller"
    }

    // Set from C++
    property bool trayAvailable: false
    property bool autoStartupAvailable: true

    // Time of the last hide caused by losing focus. The tray click that took
    // focus away would otherwise immediately reopen the popup.
    property double lastFocusHideMs: 0

    // Size to fit whatever content is actually in mainColumn, rather than a
    // guessed fixed size that can clip content as fields are added.
    width: mainColumn.implicitWidth + mainColumn.anchors.margins * 2
    height: mainColumn.implicitHeight + mainColumn.anchors.margins * 2
    title: "WorkTime"

    onClosing: function (close) {
        if (trayAvailable) {
            close.accepted = false
            window.hide()
        }
    }

    onActiveChanged: {
        if (!active && trayAvailable && visible) {
            lastFocusHideMs = Date.now()
            hide()
        }
    }

    function showPopup() {
        if (trayAvailable) {
            positionPopup()
            popupPlacement.requestPlacement()
        }
        show()
        raise()
        requestActivate()
    }

    // Places the popup next to the tray icon, inside the screen's work area
    // (so it stays clear of the taskbar or menu bar). Some tray backends
    // report no icon geometry, and Wayland ignores client-set positions, so
    // without a tray rectangle the popup goes to the work area's corner.
    function positionPopup() {
        const margin = 8
        const g = trayIcon.geometry
        const hasTrayRect = g.width > 0 && g.height > 0

        let area
        let px
        let py
        if (hasTrayRect) {
            const cx = g.x + g.width / 2
            const cy = g.y + g.height / 2
            area = screenGeometry.availableGeometryAt(Qt.point(cx, cy))
            px = cx - window.width / 2
            // Open towards the middle of the screen: above a bottom tray, below a top one.
            const screenMidY = area.y + area.height / 2
            py = cy > screenMidY ? g.y - window.height - margin
                                 : g.y + g.height + margin
        } else {
            area = screenGeometry.primaryAvailableGeometry()
            px = area.x + area.width - window.width - margin
            // The macOS menu bar is at the top, elsewhere the tray is at the bottom.
            py = Qt.platform.os === "osx" ? area.y + margin
                                          : area.y + area.height - window.height - margin
        }

        const right = area.x + area.width
        const bottom = area.y + area.height
        x = Math.max(area.x + margin, Math.min(px, right - window.width - margin))
        y = Math.max(area.y + margin, Math.min(py, bottom - window.height - margin))
    }

    Shortcut {
        sequence: "Esc"
        enabled: window.trayAvailable && window.visible
        onActivated: window.hide()
    }

    Platform.SystemTrayIcon {
        id: trayIcon
        visible: window.trayAvailable
        // A file:// path on disk (extracted from the qrc resource by
        // MainController), not a qrc:/... resource path: Qt.labs.platform's
        // Linux tray backend silently produces no icon at all when
        // icon.source points into the Qt resource system, regardless of
        // image format. The icon itself tracks controller.userStatus -
        // green while active, red on error, the default icon otherwise.
        icon.source: controller.statusIconSource
        tooltip: "WorkTime"

        // On macOS a menu attached to the status item opens on every click,
        // left or right, so there it stays detached and is opened only for
        // right-clicks. Elsewhere the attached menu already opens only on
        // right-click.
        menu: Qt.platform.os === "osx" ? null : trayMenu

        onActivated: function (reason) {
            if (reason === Platform.SystemTrayIcon.Context) {
                if (Qt.platform.os === "osx")
                    trayMenu.open()
                return
            }
            if (reason !== Platform.SystemTrayIcon.Trigger)
                return
            if (window.visible)
                window.hide()
            else if (Date.now() - window.lastFocusHideMs > 300)
                window.showPopup()
        }
    }

    Platform.Menu {
        id: trayMenu

        // On Linux the left-click opens this menu, so Show is the way to reach the window.
        Platform.MenuItem {
            objectName: "showMenuItem"
            text: "Show"
            visible: Qt.platform.os === "linux"
            onTriggered: window.showPopup()
        }
        Platform.MenuItem {
            objectName: "myOverviewMenuItem"
            text: "My overview"
            onTriggered: Qt.openUrlExternally("https://worktime.lv/user/read")
        }
        Platform.MenuItem {
            objectName: "quitMenuItem"
            text: "Quit"
            onTriggered: appQuitter.quit()
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
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                radius: 6
                color: controller.startButtonPushed ? "#2ecc71" : "#95a5a6"

                Behavior on color { ColorAnimation { duration: 150 } }
            }

            Label {
                objectName: "statusLabel"
                text: controller.startButtonPushed ? "Tracking" : "Stopped"
                font.pixelSize: 16
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                objectName: "startButton"
                text: controller.startButtonPushed ? "Stop" : "Start"
                highlighted: true
                onClicked: {
                    controller.username = usernameField.text
                    controller.password = passwordField.text
                    controller.toggleTracking()
                }
            }
        }

        Frame {
            Layout.fillWidth: true

            GridLayout {
                columns: 2
                columnSpacing: 10
                rowSpacing: 8
                anchors.fill: parent

                Label { text: "Sent messages (last 24h)" }
                Label {
                    objectName: "sentCountLabel"
                    text: controller.sentCountLast24Hours.toLocaleString(Qt.locale(), 'f', 0)
                }

                Label { text: "Sent data (last 24h)" }
                Label {
                    objectName: "sentBytesLabel"
                    text: controller.sentBytesLast24Hours.toLocaleString(Qt.locale(), 'f', 0) + " bytes"
                }
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
                    visible: window.autoStartupAvailable
                }

                GridLayout {
                    columns: 2
                    columnSpacing: 10
                    rowSpacing: 8
                    Layout.fillWidth: true
                    visible: window.autoStartupAvailable

                    Label { text: "Launch at Startup" }
                    CheckBox {
                        objectName: "autoStartupCheckBox"
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
                        id: usernameField
                        objectName: "usernameField"
                        text: controller.username
                        onEditingFinished: controller.username = text
                        Layout.fillWidth: true

                        Rectangle {
                            objectName: "usernameRejectedBorder"
                            anchors.fill: parent
                            visible: controller.credentialsRejected
                            color: "transparent"
                            radius: 4
                            border.width: 2
                            border.color: "#e74c3c"
                        }
                    }

                    Label { text: "Password" }
                    TextField {
                        id: passwordField
                        objectName: "passwordField"
                        text: controller.password
                        echoMode: TextInput.Password
                        onEditingFinished: controller.password = text
                        Layout.fillWidth: true

                        Rectangle {
                            objectName: "passwordRejectedBorder"
                            anchors.fill: parent
                            visible: controller.credentialsRejected
                            color: "transparent"
                            radius: 4
                            border.width: 2
                            border.color: "#e74c3c"
                        }
                    }
                }
            }
        }
    }

    // Outline for the frameless popup, so its edge shows on any background.
    Rectangle {
        anchors.fill: parent
        visible: window.trayAvailable
        color: "transparent"
        border.width: 1
        border.color: palette.mid
    }
}
