#include <gtest/gtest.h>

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QList>
#include <QString>

#include "core/kwinscript.hpp"
#include "qml/controller/popupplacement.hpp"

namespace {

class TestablePopupPlacement : public PopupPlacement
{
public:
    using PopupPlacement::s_forceWaylandForTesting;
    using PopupPlacement::s_gnomePlacementInterface;
    using PopupPlacement::s_gnomePlacementPath;
    using PopupPlacement::s_gnomePlacementService;
    using PopupPlacement::s_gnomeShellExtensionsPath;
    using PopupPlacement::s_gnomeShellExtensionsService;
};

class TestableKWinScript : public KWinScript
{
public:
    using KWinScript::s_serviceName;
};

// Stands in for org.gnome.Shell.Extensions: tracks every call and replies
// with a controllable bool, like the real EnableExtension/DisableExtension.
class FakeGnomeExtensionsManager : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool enableReturnValue = true;
    bool disableReturnValue = true;
    bool callsShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        calls.append(message);
        if (callsShouldFail) {
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no shell")));
        } else if (message.member() == QStringLiteral("EnableExtension")) {
            connection.send(message.createReply(enableReturnValue));
        } else if (message.member() == QStringLiteral("DisableExtension")) {
            connection.send(message.createReply(disableReturnValue));
        }
        return true;
    }
};

// Stands in for this app's own GNOME Shell extension's Placement interface.
class FakeGnomePlacement : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool callsShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        calls.append(message);
        if (callsShouldFail)
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no shell")));
        else
            connection.send(message.createReply());
        return true;
    }
};

// Minimal stand-in for org.kde.KWin's /Scripting object - just enough for
// PopupPlacement::start()/stop()'s KDE branch (via KWinScript::load/unload).
class FakeKWinScripting : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        calls.append(message);
        if (message.member() == QStringLiteral("isScriptLoaded"))
            connection.send(message.createReply(false));
        else if (message.member() == QStringLiteral("loadScript"))
            connection.send(message.createReply(42));
        else if (message.member() == QStringLiteral("unloadScript"))
            connection.send(message.createReply());
        return true;
    }
};

// Serves the GNOME fakes on this process's own session-bus connection and
// points PopupPlacement at them, so tests never talk to a real GNOME Shell -
// same technique as UtilitiesAutostartTest/KWinScriptTest.
class PopupPlacementTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/gnome/Shell/Extensions", &m_extensionsManager));
        ASSERT_TRUE(m_bus.registerVirtualObject("/io/github/filesfm/worktime/Placement", &m_placement));

        m_previousForceWayland = TestablePopupPlacement::s_forceWaylandForTesting;
        m_previousExtensionsService = TestablePopupPlacement::s_gnomeShellExtensionsService;
        m_previousExtensionsPath = TestablePopupPlacement::s_gnomeShellExtensionsPath;
        m_previousPlacementService = TestablePopupPlacement::s_gnomePlacementService;
        m_previousPlacementPath = TestablePopupPlacement::s_gnomePlacementPath;
        m_previousPlacementInterface = TestablePopupPlacement::s_gnomePlacementInterface;
        m_previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");

        TestablePopupPlacement::s_forceWaylandForTesting = true;
        TestablePopupPlacement::s_gnomeShellExtensionsService = m_bus.baseService();
        TestablePopupPlacement::s_gnomeShellExtensionsPath = QStringLiteral("/org/gnome/Shell/Extensions");
        TestablePopupPlacement::s_gnomePlacementService = m_bus.baseService();
        TestablePopupPlacement::s_gnomePlacementPath = QStringLiteral("/io/github/filesfm/worktime/Placement");
        TestablePopupPlacement::s_gnomePlacementInterface = QStringLiteral("io.github.filesfm.worktime.Placement");
    }

    void TearDown() override
    {
        qputenv("XDG_CURRENT_DESKTOP", m_previousDesktop.toUtf8());
        if (!m_bus.isConnected())
            return;

        TestablePopupPlacement::s_forceWaylandForTesting = m_previousForceWayland;
        TestablePopupPlacement::s_gnomeShellExtensionsService = m_previousExtensionsService;
        TestablePopupPlacement::s_gnomeShellExtensionsPath = m_previousExtensionsPath;
        TestablePopupPlacement::s_gnomePlacementService = m_previousPlacementService;
        TestablePopupPlacement::s_gnomePlacementPath = m_previousPlacementPath;
        TestablePopupPlacement::s_gnomePlacementInterface = m_previousPlacementInterface;
        m_bus.unregisterObject("/org/gnome/Shell/Extensions");
        m_bus.unregisterObject("/io/github/filesfm/worktime/Placement");
    }

    QDBusConnection m_bus{QString()};
    FakeGnomeExtensionsManager m_extensionsManager;
    FakeGnomePlacement m_placement;
    bool m_previousForceWayland = false;
    QString m_previousExtensionsService;
    QString m_previousExtensionsPath;
    QString m_previousPlacementService;
    QString m_previousPlacementPath;
    QString m_previousPlacementInterface;
    QString m_previousDesktop;
};

} // namespace

TEST_F(PopupPlacementTest, StartDoesNothingWhenNotWayland)
{
    TestablePopupPlacement::s_forceWaylandForTesting = false;
    qputenv("XDG_CURRENT_DESKTOP", "gnome");

    PopupPlacement placement;
    placement.start();

    EXPECT_TRUE(m_extensionsManager.calls.isEmpty());
}

TEST_F(PopupPlacementTest, StartOnGnomeEnablesTheExtension)
{
    qputenv("XDG_CURRENT_DESKTOP", "gnome");
    m_extensionsManager.enableReturnValue = true;

    PopupPlacement placement;
    placement.start();

    bool sawEnable = false;
    for (const QDBusMessage &call : m_extensionsManager.calls) {
        if (call.member() == QStringLiteral("EnableExtension"))
            sawEnable = true;
    }
    EXPECT_TRUE(sawEnable);
}

TEST_F(PopupPlacementTest, RequestPlacementCallsPlaceNextPopupWhenGnomeActive)
{
    qputenv("XDG_CURRENT_DESKTOP", "gnome");
    m_extensionsManager.enableReturnValue = true;

    PopupPlacement placement;
    placement.start();
    placement.requestPlacement();

    ASSERT_EQ(m_placement.calls.size(), 1);
    EXPECT_EQ(m_placement.calls.constFirst().member(), QStringLiteral("PlaceNextPopup"));
}

TEST_F(PopupPlacementTest, RequestPlacementDoesNothingWhenGnomeNotActive)
{
    qputenv("XDG_CURRENT_DESKTOP", "gnome");
    m_extensionsManager.enableReturnValue = false; // EnableExtension "fails" -> m_gnomeActive stays false

    PopupPlacement placement;
    placement.start();
    placement.requestPlacement();

    EXPECT_TRUE(m_placement.calls.isEmpty());
}

TEST_F(PopupPlacementTest, StartOnKdeLoadsTheKWinScriptAndStopUnloadsIt)
{
    qputenv("XDG_CURRENT_DESKTOP", "kde");

    FakeKWinScripting kwinScripting;
    ASSERT_TRUE(m_bus.registerVirtualObject("/Scripting", &kwinScripting));
    const QString previousKWinService = TestableKWinScript::s_serviceName;
    TestableKWinScript::s_serviceName = m_bus.baseService();

    PopupPlacement placement;
    placement.start();

    bool sawLoadScript = false;
    for (const QDBusMessage &call : kwinScripting.calls) {
        if (call.member() == QStringLiteral("loadScript"))
            sawLoadScript = true;
    }
    EXPECT_TRUE(sawLoadScript);

    placement.stop();

    bool sawUnloadScript = false;
    for (const QDBusMessage &call : kwinScripting.calls) {
        if (call.member() == QStringLiteral("unloadScript"))
            sawUnloadScript = true;
    }
    EXPECT_TRUE(sawUnloadScript);

    TestableKWinScript::s_serviceName = previousKWinService;
    m_bus.unregisterObject("/Scripting");
}

TEST_F(PopupPlacementTest, StopWithoutKdeActiveIsNoOp)
{
    PopupPlacement placement;

    EXPECT_NO_FATAL_FAILURE(placement.stop());
}

TEST_F(PopupPlacementTest, StartOnGnomeHandlesAFailedEnableExtensionCall)
{
    qputenv("XDG_CURRENT_DESKTOP", "gnome");
    m_extensionsManager.callsShouldFail = true;

    PopupPlacement placement;
    EXPECT_NO_FATAL_FAILURE(placement.start());
}

TEST_F(PopupPlacementTest, RequestPlacementHandlesAFailedPlaceNextPopupCall)
{
    qputenv("XDG_CURRENT_DESKTOP", "gnome");
    m_extensionsManager.enableReturnValue = true;

    PopupPlacement placement;
    placement.start();

    m_placement.callsShouldFail = true;
    EXPECT_NO_FATAL_FAILURE(placement.requestPlacement());
}
