import QtQuick
import QtTest
import Worktime

// Tests for the main window defined in src/qml/Main.qml. The controller is
// the MainController test double from test/gui/qml/MainController.qml.
TestCase {
    name: "MainWindow"

    Component {
        id: mainComponent
        Main {}
    }

    // Each test gets a fresh window, which is destroyed after the test.
    property var win
    property var controller

    function init() {
        win = createTemporaryObject(mainComponent, null, { trayAvailable: false })
        verify(win !== null)
        controller = findChild(win, "controller")
        verify(controller !== null)
        waitForRendering(win.contentItem)
    }

    function item(objectName, root) {
        const found = findChild(root || win, objectName)
        verify(found !== null, "no item named " + objectName)
        return found
    }

    // Types text into a field one key at a time, as a user would.
    function typeInto(field, text) {
        mouseClick(field)
        for (let i = 0; i < text.length; ++i)
            keyClick(text.charAt(i))
    }

    function test_windowIsVisibleWithoutSystemTray() {
        compare(win.visible, true)
    }

    function test_windowIsHiddenWhenSystemTrayIsAvailable() {
        const tray = createTemporaryObject(mainComponent, null, { trayAvailable: true })
        verify(tray !== null)
        compare(tray.visible, false)
    }

    function test_startsStopped() {
        compare(item("statusLabel").text, "Stopped")
        compare(item("startButton").text, "Start")
    }

    function test_startButtonStartsAndStopsTracking() {
        mouseClick(item("startButton"))
        compare(controller.toggleCount, 1)
        compare(item("statusLabel").text, "Tracking")
        compare(item("startButton").text, "Stop")

        mouseClick(item("startButton"))
        compare(controller.toggleCount, 2)
        compare(item("statusLabel").text, "Stopped")
        compare(item("startButton").text, "Start")
    }

    function test_startButtonSavesTypedCredentials() {
        typeInto(item("usernameField"), "alice")
        typeInto(item("passwordField"), "hunter2")

        mouseClick(item("startButton"))

        compare(controller.username, "alice")
        compare(controller.password, "hunter2")
    }

    function test_sentStatsAreShown() {
        controller.sentCountLast24Hours = 1234
        controller.sentBytesLast24Hours = 5678
        compare(item("sentCountLabel").text, Number(1234).toLocaleString(Qt.locale(), 'f', 0))
        compare(item("sentBytesLabel").text,
                Number(5678).toLocaleString(Qt.locale(), 'f', 0) + " bytes")
    }

    function test_escHidesWindowWhenTrayAvailable() {
        const tray = createTemporaryObject(mainComponent, null, { trayAvailable: true })
        verify(tray !== null)
        tray.showPopup()
        tryCompare(tray, "visible", true)
        tryCompare(tray, "active", true)

        keyClick(Qt.Key_Escape)

        tryCompare(tray, "visible", false)
    }

    function test_escDoesNothingWithoutTray() {
        compare(win.visible, true)

        keyClick(Qt.Key_Escape)

        compare(win.visible, true)
    }

    function test_showMenuItemIsLinuxOnlyAndOpensWindow() {
        const tray = createTemporaryObject(mainComponent, null, { trayAvailable: true })
        verify(tray !== null)
        compare(tray.visible, false)

        const show = item("showMenuItem", tray)

        if (Qt.platform.os === "linux") {
            compare(show.visible, true)
            show.triggered()
            tryCompare(tray, "visible", true)
        } else {
            compare(show.visible, false)
        }
    }

    function test_myOverviewMenuItemIsAvailable() {
        const overview = item("myOverviewMenuItem")
        compare(overview.visible, true)
        compare(overview.enabled, true)
    }

    function test_quitMenuItemQuitsTheApplication() {
        compare(appQuitter.quitCount, 0)

        item("quitMenuItem").triggered()

        compare(appQuitter.quitCount, 1)
    }
}
