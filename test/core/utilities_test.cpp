#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QScreen>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QUrl>

#include "core/utilities.hpp"

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)
#    include <QDBusArgument>
#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusObjectPath>
#    include <QDBusVirtualObject>
#    include <QVariantMap>
#endif

#if defined(Q_OS_LINUX)
#    include <QDBusConnection>
#    include <QDBusError>
#    include <QDBusMessage>
#    include <QDBusVariant>
#    include <QDBusVirtualObject>
#    include <QVariantMap>
#endif

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_WINDOWS)
#    include <QCoreApplication>
#endif

#if defined(Q_OS_WINDOWS)
#    include <QSettings>
#endif

TEST(UtilitiesTest, FocusedApplicationNameDoesNotCrash)
{
    // No assumption on the returned value: whether a name is available
    // depends on the desktop session (X11 vs Wayland) and which window (if
    // any) currently has focus. This only guards against the call itself
    // crashing.
    (void)Utilities::focusedWindowTitle();
}

TEST(UtilitiesTest, ExtractResourceToDiskCopiesContentAndReturnsFileUrl)
{
    QTemporaryDir sourceDir;
    ASSERT_TRUE(sourceDir.isValid());
    const QString sourcePath = sourceDir.filePath("source.txt");
    QFile sourceFile(sourcePath);
    ASSERT_TRUE(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("hello");
    sourceFile.close();

    const QUrl url = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-copy.txt");

    ASSERT_TRUE(url.isLocalFile());
    EXPECT_EQ(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                  .filePath("worktime-utilities-test-copy.txt"),
              url.toLocalFile());

    QFile destinationFile(url.toLocalFile());
    ASSERT_TRUE(destinationFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(destinationFile.readAll(), QByteArray("hello"));
    destinationFile.close();

    QFile::remove(url.toLocalFile());
}

TEST(UtilitiesTest, ExtractResourceToDiskOverwritesAnExistingDestination)
{
    QTemporaryDir sourceDir;
    ASSERT_TRUE(sourceDir.isValid());
    const QString sourcePath = sourceDir.filePath("source.txt");

    QFile firstSource(sourcePath);
    ASSERT_TRUE(firstSource.open(QIODevice::WriteOnly));
    firstSource.write("first");
    firstSource.close();
    const QUrl firstUrl = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-overwrite.txt");

    QFile secondSource(sourcePath);
    ASSERT_TRUE(secondSource.open(QIODevice::WriteOnly | QIODevice::Truncate));
    secondSource.write("second");
    secondSource.close();
    const QUrl secondUrl = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-overwrite.txt");

    QFile destinationFile(secondUrl.toLocalFile());
    ASSERT_TRUE(destinationFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(destinationFile.readAll(), QByteArray("second"));
    destinationFile.close();

    QFile::remove(firstUrl.toLocalFile());
}

TEST(UtilitiesTest, TruncateUtf8SafeValueUnderLimitIsUnchanged)
{
    EXPECT_EQ(Utilities::truncateUtf8Safe("hello", 255), QStringLiteral("hello"));
}

TEST(UtilitiesTest, TruncateUtf8SafeValueAtLimitIsUnchanged)
{
    const QString value(255, QLatin1Char('a'));
    EXPECT_EQ(Utilities::truncateUtf8Safe(value, 255), value);
}

TEST(UtilitiesTest, TruncateUtf8SafeValueOverLimitIsTruncatedToExactLength)
{
    const QString value(300, QLatin1Char('a'));
    const QString truncated = Utilities::truncateUtf8Safe(value, 255);

    EXPECT_EQ(truncated.length(), 255);
    EXPECT_EQ(truncated, QString(255, QLatin1Char('a')));
}

// The 255th code point is an astral character (U+1F600, outside the Basic
// Multilingual Plane) represented in UTF-16 as a surrogate pair. Truncating
// at a UTF-16 code-unit boundary instead of a code-point boundary would slice
// the pair in half, producing an unpaired surrogate that can't be encoded to
// valid UTF-8.
TEST(UtilitiesTest, TruncateUtf8SafeDoesNotSplitAnAstralCharacterAtTheBoundary)
{
    QString value(254, QLatin1Char('a'));
    value.append(QChar::highSurrogate(0x1F600));
    value.append(QChar::lowSurrogate(0x1F600));
    value.append(QLatin1Char('z'));

    const QString truncated = Utilities::truncateUtf8Safe(value, 255);

    ASSERT_EQ(truncated.toUcs4().size(), 255);
    EXPECT_TRUE(truncated.toUtf8().size() > 0);
    EXPECT_FALSE(truncated.endsWith(QChar::highSurrogate(0x1F600)));
}

TEST(UtilitiesTest, TruncateUtf8SafeEmptyValueIsUnchanged)
{
    EXPECT_EQ(Utilities::truncateUtf8Safe(QString(), 255), QString());
}

#if defined(Q_OS_LINUX)

TEST(UtilitiesTest, FocusedWindowTitleReturnsEmptyOnUnknownDesktop)
{
    const QString previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    qputenv("XDG_CURRENT_DESKTOP", "someunknowndesktop");

    EXPECT_EQ(Utilities::focusedWindowTitle(), QString());

    qputenv("XDG_CURRENT_DESKTOP", previousDesktop.toUtf8());
}

TEST(UtilitiesTest, FocusedWindowTitleDelegatesToKDEFocusedWindowTitleUnderKde)
{
    const QString previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    qputenv("XDG_CURRENT_DESKTOP", "kde");

    // KDEFocusedWindowTitle has its own dedicated tests; this only guards
    // the one-line delegation itself against crashing.
    (void)Utilities::focusedWindowTitle();

    qputenv("XDG_CURRENT_DESKTOP", previousDesktop.toUtf8());
}

namespace {

class TestableUtilitiesFocusedWindow : public Utilities
{
public:
    using Utilities::s_cinnamonService;
    using Utilities::s_gnomeShellService;
};

// Stands in for org.gnome.Shell's FocusedWindow extension: replies with a
// JSON string, {"title": "..."}, the way the real extension does.
class FakeGnomeFocusedWindow : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool shouldFail = false;
    QString jsonReply = QStringLiteral("{}");

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        if (shouldFail)
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no shell")));
        else
            connection.send(message.createReply(QVariant::fromValue(jsonReply)));
        return true;
    }
};

class UtilitiesFocusedWindowGnomeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/gnome/shell/extensions/FocusedWindow", &m_fake));
        m_previousService = TestableUtilitiesFocusedWindow::s_gnomeShellService;
        TestableUtilitiesFocusedWindow::s_gnomeShellService = m_bus.baseService();
        m_previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
        qputenv("XDG_CURRENT_DESKTOP", "gnome");
    }

    void TearDown() override
    {
        qputenv("XDG_CURRENT_DESKTOP", m_previousDesktop.toUtf8());
        if (!m_bus.isConnected())
            return;

        TestableUtilitiesFocusedWindow::s_gnomeShellService = m_previousService;
        m_bus.unregisterObject("/org/gnome/shell/extensions/FocusedWindow");
    }

    QDBusConnection m_bus{QString()};
    FakeGnomeFocusedWindow m_fake;
    QString m_previousService;
    QString m_previousDesktop;
};

} // namespace

TEST_F(UtilitiesFocusedWindowGnomeTest, ReturnsTitleFromJsonReply)
{
    m_fake.jsonReply = QStringLiteral(R"({"title": "My Window"})");
    EXPECT_EQ(Utilities::focusedWindowTitle(), QStringLiteral("My Window"));
}

TEST_F(UtilitiesFocusedWindowGnomeTest, ReturnsEmptyWhenCallFails)
{
    m_fake.shouldFail = true;
    EXPECT_EQ(Utilities::focusedWindowTitle(), QString());
}

namespace {

// Stands in for org.Cinnamon's Eval method: replies (success, result) the
// way the real Cinnamon shell does, with result a raw JS expression result.
class FakeCinnamonEval : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool shouldFail = false;
    bool success = true;
    QString result;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        if (shouldFail)
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no cinnamon")));
        else
            connection.send(message.createReply(QList<QVariant>{QVariant(success), QVariant(result)}));
        return true;
    }
};

class UtilitiesFocusedWindowCinnamonTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/Cinnamon", &m_fake));
        m_previousService = TestableUtilitiesFocusedWindow::s_cinnamonService;
        TestableUtilitiesFocusedWindow::s_cinnamonService = m_bus.baseService();
        m_previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
        qputenv("XDG_CURRENT_DESKTOP", "cinnamon");
    }

    void TearDown() override
    {
        qputenv("XDG_CURRENT_DESKTOP", m_previousDesktop.toUtf8());
        if (!m_bus.isConnected())
            return;

        TestableUtilitiesFocusedWindow::s_cinnamonService = m_previousService;
        m_bus.unregisterObject("/org/Cinnamon");
    }

    QDBusConnection m_bus{QString()};
    FakeCinnamonEval m_fake;
    QString m_previousService;
    QString m_previousDesktop;
};

} // namespace

TEST_F(UtilitiesFocusedWindowCinnamonTest, ReturnsTitleOnSuccess)
{
    m_fake.success = true;
    m_fake.result = QStringLiteral("\"My Window\""); // Eval returns a JS string literal, unquoted here
    EXPECT_EQ(Utilities::focusedWindowTitle(), QStringLiteral("My Window"));
}

TEST_F(UtilitiesFocusedWindowCinnamonTest, ReturnsEmptyWhenEvalReportsFailure)
{
    m_fake.success = false;
    m_fake.result = QStringLiteral("some error");
    EXPECT_EQ(Utilities::focusedWindowTitle(), QString());
}

TEST_F(UtilitiesFocusedWindowCinnamonTest, ReturnsEmptyWhenCallFails)
{
    m_fake.shouldFail = true;
    EXPECT_EQ(Utilities::focusedWindowTitle(), QString());
}

#endif // defined(Q_OS_LINUX)

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)

namespace {

class TestableUtilities : public Utilities
{
public:
    using Utilities::s_autostartDirectory;
    using Utilities::s_autostartPortalService;
};

/*
 * Stands in for xdg-desktop-portal: records every message it receives and
 * replies the way the real portal does, with a request object path.
 */
class FakeBackgroundPortal : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        connection.send(message.createReply(
            QVariant::fromValue(QDBusObjectPath("/org/freedesktop/portal/desktop/request/1_0/worktime"))));
        return true;
    }
};

/*
 * Serves FakeBackgroundPortal on this process's own session-bus connection
 * and points autostart() at it, so tests never change the real autostart
 * configuration. Qt delivers calls addressed to a connection's own unique
 * name in-process, so autostart()'s blocking call reaches the fake directly.
 */
class UtilitiesAutostartTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/freedesktop/portal/desktop", &m_portal));
        m_previousService = TestableUtilities::s_autostartPortalService;
        TestableUtilities::s_autostartPortalService = m_bus.baseService();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableUtilities::s_autostartPortalService = m_previousService;
        m_bus.unregisterObject("/org/freedesktop/portal/desktop");
    }

    //! Options map (second argument) of the single call the fake portal received.
    QVariantMap onlyRequestOptions()
    {
        EXPECT_EQ(m_portal.calls.size(), 1);
        if (m_portal.calls.size() != 1 || m_portal.calls.constFirst().arguments().size() != 2)
            return {};
        return qdbus_cast<QVariantMap>(m_portal.calls.constFirst().arguments().at(1));
    }

    QDBusConnection m_bus{QString()};
    FakeBackgroundPortal m_portal;
    QString m_previousService;
};

} // namespace

TEST_F(UtilitiesAutostartTest, EnablingRequestsAutostartFromTheBackgroundPortal)
{
    Utilities::autostart(true);

    ASSERT_EQ(m_portal.calls.size(), 1);
    EXPECT_EQ(m_portal.calls.constFirst().interface(), QStringLiteral("org.freedesktop.portal.Background"));
    EXPECT_EQ(m_portal.calls.constFirst().member(), QStringLiteral("RequestBackground"));
    EXPECT_TRUE(onlyRequestOptions().value("autostart").toBool());
}

TEST_F(UtilitiesAutostartTest, DisablingRevokesAutostartFromTheBackgroundPortal)
{
    Utilities::autostart(false);

    const QVariantMap options = onlyRequestOptions();
    ASSERT_TRUE(options.contains("autostart"));
    EXPECT_FALSE(options.value("autostart").toBool());
}

TEST_F(UtilitiesAutostartTest, CommandLineUsesFlatpakRunWhenFlatpakIdIsSet)
{
    qputenv("FLATPAK_ID", "io.github.filesfm.worktime");

    Utilities::autostart(true);

    const QStringList commandLine = onlyRequestOptions().value("commandline").toStringList();
    EXPECT_EQ(commandLine, QStringList({"flatpak", "run", "io.github.filesfm.worktime"}));

    qunsetenv("FLATPAK_ID");
}

TEST_F(UtilitiesAutostartTest, CommandLineUsesSnapRunWhenSnapIsSet)
{
    qputenv("SNAP", "1");

    Utilities::autostart(true);

    const QStringList commandLine = onlyRequestOptions().value("commandline").toStringList();
    EXPECT_EQ(commandLine, QStringList({"snap", "run", "files-fm-worktime"}));

    qunsetenv("SNAP");
}

namespace {

class UtilitiesAutostartStateTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(m_directory.isValid());
        m_previousDirectory = TestableUtilities::s_autostartDirectory;
        TestableUtilities::s_autostartDirectory = m_directory.path();
    }

    void TearDown() override { TestableUtilities::s_autostartDirectory = m_previousDirectory; }

    void writeDesktopEntry(const QString &fileName, const QByteArray &content)
    {
        QFile file(m_directory.filePath(fileName));
        ASSERT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(content);
    }

    QTemporaryDir m_directory;
    QString m_previousDirectory;
};

} // namespace

TEST_F(UtilitiesAutostartStateTest, NoDesktopEntryMeansDisabled)
{
    EXPECT_FALSE(Utilities::isAutostartEnabled());
}

TEST_F(UtilitiesAutostartStateTest, DesktopEntryMeansEnabled)
{
    writeDesktopEntry("io.github.filesfm.worktime.desktop", "[Desktop Entry]\nType=Application\nExec=worktime\n");

    EXPECT_TRUE(Utilities::isAutostartEnabled());
}

TEST_F(UtilitiesAutostartStateTest, HiddenDesktopEntryMeansDisabled)
{
    writeDesktopEntry("io.github.filesfm.worktime.desktop",
                      "[Desktop Entry]\nType=Application\nExec=worktime\nHidden=true\n");

    EXPECT_FALSE(Utilities::isAutostartEnabled());
}

TEST_F(UtilitiesAutostartStateTest, OtherApplicationsDesktopEntryIsIgnored)
{
    writeDesktopEntry("org.example.other.desktop", "[Desktop Entry]\nType=Application\nExec=other\n");

    EXPECT_FALSE(Utilities::isAutostartEnabled());
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)

#if defined(Q_OS_LINUX)

namespace {

class TestableUtilitiesGnomeExtensions : public Utilities
{
public:
    using Utilities::s_gnomeExtensionsService;
};

// Stands in for org.gnome.Shell.Extensions: tracks calls and replies the
// way the real extension manager does to GetExtensionInfo (controlled via
// extensionInfo) and to InstallRemoteExtension/EnableExtension (void reply).
class FakeGnomeExtensionsManager : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    QVariantMap extensionInfo; // empty map = "not installed"
    bool callsShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        if (callsShouldFail) {
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no shell")));
        } else if (message.member() == QStringLiteral("GetExtensionInfo")) {
            connection.send(message.createReply(QVariant::fromValue(extensionInfo)));
        } else {
            connection.send(message.createReply());
        }
        return true;
    }
};

class UtilitiesGnomeExtensionsTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/gnome/Shell/Extensions", &m_manager));
        m_previousService = TestableUtilitiesGnomeExtensions::s_gnomeExtensionsService;
        TestableUtilitiesGnomeExtensions::s_gnomeExtensionsService = m_bus.baseService();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableUtilitiesGnomeExtensions::s_gnomeExtensionsService = m_previousService;
        m_bus.unregisterObject("/org/gnome/Shell/Extensions");
    }

    QDBusConnection m_bus{QString()};
    FakeGnomeExtensionsManager m_manager;
    QString m_previousService;
};

} // namespace

TEST_F(UtilitiesGnomeExtensionsTest, NotInstalledWhenExtensionInfoIsEmpty)
{
    EXPECT_FALSE(Utilities::isGNOMEFocusedWindowDBusInstalled());
}

TEST_F(UtilitiesGnomeExtensionsTest, InstalledWhenExtensionInfoIsNonEmpty)
{
    m_manager.extensionInfo = {{"uuid", "focused-window-dbus@flexagoon.com"}};
    EXPECT_TRUE(Utilities::isGNOMEFocusedWindowDBusInstalled());
}

TEST_F(UtilitiesGnomeExtensionsTest, NotEnabledWhenStateIsNotOne)
{
    m_manager.extensionInfo = {{"state", 0u}};
    EXPECT_FALSE(Utilities::isGNOMEFocusedWindowDBusEnabled());
}

TEST_F(UtilitiesGnomeExtensionsTest, EnabledWhenStateIsOne)
{
    m_manager.extensionInfo = {{"state", 1u}};
    EXPECT_TRUE(Utilities::isGNOMEFocusedWindowDBusEnabled());
}

TEST_F(UtilitiesGnomeExtensionsTest, InstallCallsInstallRemoteExtension)
{
    Utilities::installGNOMEFocusedWindowDBus();

    ASSERT_EQ(m_manager.calls.size(), 1);
    EXPECT_EQ(m_manager.calls.constFirst().member(), QStringLiteral("InstallRemoteExtension"));
}

TEST_F(UtilitiesGnomeExtensionsTest, EnableCallsEnableExtension)
{
    Utilities::enableGNOMEFocusedWindowDBus();

    ASSERT_EQ(m_manager.calls.size(), 1);
    EXPECT_EQ(m_manager.calls.constFirst().member(), QStringLiteral("EnableExtension"));
}

TEST_F(UtilitiesGnomeExtensionsTest, NotInstalledWhenCallFails)
{
    m_manager.callsShouldFail = true;
    EXPECT_FALSE(Utilities::isGNOMEFocusedWindowDBusInstalled());
}

TEST_F(UtilitiesGnomeExtensionsTest, NotEnabledWhenCallFails)
{
    m_manager.callsShouldFail = true;
    EXPECT_FALSE(Utilities::isGNOMEFocusedWindowDBusEnabled());
}

TEST_F(UtilitiesGnomeExtensionsTest, InstallHandlesAFailedCall)
{
    m_manager.callsShouldFail = true;
    EXPECT_NO_FATAL_FAILURE(Utilities::installGNOMEFocusedWindowDBus());
}

TEST_F(UtilitiesGnomeExtensionsTest, EnableHandlesAFailedCall)
{
    m_manager.callsShouldFail = true;
    EXPECT_NO_FATAL_FAILURE(Utilities::enableGNOMEFocusedWindowDBus());
}

namespace {

class TestableUtilitiesNotifications : public Utilities
{
public:
    using Utilities::s_notificationsService;
};

// Stands in for org.freedesktop.Notifications: tracks the call and replies
// with a notification id, like the real daemon does.
class FakeNotificationsService : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool callShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        if (callShouldFail)
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no notifications")));
        else
            connection.send(message.createReply(QVariant::fromValue(quint32(1))));
        return true;
    }
};

class UtilitiesNotificationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/freedesktop/Notifications", &m_service));
        m_previousService = TestableUtilitiesNotifications::s_notificationsService;
        TestableUtilitiesNotifications::s_notificationsService = m_bus.baseService();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableUtilitiesNotifications::s_notificationsService = m_previousService;
        m_bus.unregisterObject("/org/freedesktop/Notifications");
    }

    QDBusConnection m_bus{QString()};
    FakeNotificationsService m_service;
    QString m_previousService;
};

} // namespace

TEST_F(UtilitiesNotificationTest, ShowNotificationSendsNotifyWithTitleAndMessage)
{
    Utilities::showNotification(QStringLiteral("Title"), QStringLiteral("Message"));

    ASSERT_EQ(m_service.calls.size(), 1);
    const QDBusMessage &call = m_service.calls.constFirst();
    EXPECT_EQ(call.member(), QStringLiteral("Notify"));
    ASSERT_GE(call.arguments().size(), 5);
    EXPECT_EQ(call.arguments().at(3).toString(), QStringLiteral("Title"));
    EXPECT_EQ(call.arguments().at(4).toString(), QStringLiteral("Message"));
}

TEST_F(UtilitiesNotificationTest, ShowNotificationHandlesAFailedCall)
{
    m_service.callShouldFail = true;
    EXPECT_NO_FATAL_FAILURE(Utilities::showNotification(QStringLiteral("Title"), QStringLiteral("Message")));
}

namespace {

class TestableUtilitiesClockSync : public Utilities
{
public:
    using Utilities::s_clockSyncService;
};

// Stands in for org.freedesktop.timedate1 on the system bus: replies to
// Properties.Get("NTP") the way the real daemon does, or with an error, or
// with an unexpected (non-variant) reply type.
class FakeTimedate : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool ntpEnabled = true;
    bool replyShouldFail = false;
    bool replyWithUnexpectedType = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        if (replyShouldFail) {
            connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no timedate1")));
        } else if (replyWithUnexpectedType) {
            connection.send(message.createReply(QVariant::fromValue(ntpEnabled)));
        } else {
            connection.send(message.createReply(QVariant::fromValue(QDBusVariant(QVariant(ntpEnabled)))));
        }
        return true;
    }
};

// systemBus(), not sessionBus(): isClockSyncEnabled() asks timedate1 there.
// Registering a virtual object under this process's own unique name doesn't
// need any special system-bus policy grant (only owning a well-known name
// does), so this works the same way the session-bus fakes elsewhere do.
class UtilitiesClockSyncLinuxTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::systemBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus system bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/freedesktop/timedate1", &m_timedate));
        m_previousService = TestableUtilitiesClockSync::s_clockSyncService;
        TestableUtilitiesClockSync::s_clockSyncService = m_bus.baseService();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableUtilitiesClockSync::s_clockSyncService = m_previousService;
        m_bus.unregisterObject("/org/freedesktop/timedate1");
    }

    QDBusConnection m_bus{QString()};
    FakeTimedate m_timedate;
    QString m_previousService;
};

} // namespace

TEST_F(UtilitiesClockSyncLinuxTest, NtpTrueMeansSyncIsOn)
{
    m_timedate.ntpEnabled = true;
    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncLinuxTest, NtpFalseMeansSyncIsOff)
{
    m_timedate.ntpEnabled = false;
    EXPECT_FALSE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncLinuxTest, FailedCallFailsOpenAsEnabled)
{
    m_timedate.replyShouldFail = true;
    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncLinuxTest, UnexpectedReplyTypeFailsOpenAsEnabled)
{
    m_timedate.replyWithUnexpectedType = true;
    m_timedate.ntpEnabled = false;
    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

#endif // defined(Q_OS_LINUX)

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_WINDOWS)

namespace {

class TestableUtilities : public Utilities
{
public:
    using Utilities::s_autostartRegistryKey;
};

class UtilitiesAutostartTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previousKey = TestableUtilities::s_autostartRegistryKey;
        TestableUtilities::s_autostartRegistryKey = QStringLiteral(
            "HKEY_CURRENT_USER\\Software\\Files.fm\\WorktimeAutostartTest");
        removeTestKey();
    }

    void TearDown() override
    {
        removeTestKey();
        TestableUtilities::s_autostartRegistryKey = m_previousKey;
    }

    static void removeTestKey()
    {
        QSettings parent(QStringLiteral("HKEY_CURRENT_USER\\Software\\Files.fm"), QSettings::NativeFormat);
        parent.remove(QStringLiteral("WorktimeAutostartTest"));
    }

    QString m_previousKey;
};

} // namespace

TEST_F(UtilitiesAutostartTest, EnablingAddsARunEntryForTheExecutable)
{
    Utilities::autostart(true);

    QSettings registry(TestableUtilities::s_autostartRegistryKey, QSettings::NativeFormat);
    EXPECT_EQ(registry.value("WorkTime").toString(), QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
}

TEST_F(UtilitiesAutostartTest, DisablingRemovesTheRunEntry)
{
    Utilities::autostart(true);
    Utilities::autostart(false);

    QSettings registry(TestableUtilities::s_autostartRegistryKey, QSettings::NativeFormat);
    EXPECT_FALSE(registry.contains("WorkTime"));
}

TEST_F(UtilitiesAutostartTest, IsEnabledAfterEnabling)
{
    Utilities::autostart(true);

    EXPECT_TRUE(Utilities::isAutostartEnabled());
}

TEST_F(UtilitiesAutostartTest, IsDisabledAfterDisabling)
{
    Utilities::autostart(true);
    Utilities::autostart(false);

    EXPECT_FALSE(Utilities::isAutostartEnabled());
}

TEST_F(UtilitiesAutostartTest, EntryForAnotherExecutableMeansDisabled)
{
    QSettings registry(TestableUtilities::s_autostartRegistryKey, QSettings::NativeFormat);
    registry.setValue("WorkTime", QStringLiteral("C:\\Elsewhere\\worktime.exe"));
    registry.sync();

    EXPECT_FALSE(Utilities::isAutostartEnabled());
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_WINDOWS)

#if defined(Q_OS_WINDOWS)

namespace {

class TestableClockSyncUtilities : public Utilities
{
public:
    using Utilities::s_clockSyncRegistryKey;
};

class UtilitiesClockSyncTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previousKey = TestableClockSyncUtilities::s_clockSyncRegistryKey;
        TestableClockSyncUtilities::s_clockSyncRegistryKey = QStringLiteral(
            "HKEY_CURRENT_USER\\Software\\Files.fm\\WorktimeClockSyncTest");
        removeTestKey();
    }

    void TearDown() override
    {
        removeTestKey();
        TestableClockSyncUtilities::s_clockSyncRegistryKey = m_previousKey;
    }

    static void removeTestKey()
    {
        QSettings parent(QStringLiteral("HKEY_CURRENT_USER\\Software\\Files.fm"), QSettings::NativeFormat);
        parent.remove(QStringLiteral("WorktimeClockSyncTest"));
    }

    QString m_previousKey;
};

} // namespace

TEST_F(UtilitiesClockSyncTest, TypeNtpMeansSyncIsOn)
{
    QSettings registry(TestableClockSyncUtilities::s_clockSyncRegistryKey, QSettings::NativeFormat);
    registry.setValue("Type", "NTP");
    registry.sync();

    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncTest, TypeNoSyncMeansSyncIsOff)
{
    QSettings registry(TestableClockSyncUtilities::s_clockSyncRegistryKey, QSettings::NativeFormat);
    registry.setValue("Type", "NoSync");
    registry.sync();

    EXPECT_FALSE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncTest, TypeNt5dsMeansSyncIsOn)
{
    // A domain-joined machine syncs from the domain hierarchy instead of a
    // public NTP server - still automatic sync, just via a different source.
    QSettings registry(TestableClockSyncUtilities::s_clockSyncRegistryKey, QSettings::NativeFormat);
    registry.setValue("Type", "NT5DS");
    registry.sync();

    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

TEST_F(UtilitiesClockSyncTest, MissingValueFailsOpenAsEnabled)
{
    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

#endif // defined(Q_OS_WINDOWS)

#if defined(Q_OS_MACOS)

namespace {

class TestableClockSyncUtilities : public Utilities
{
public:
    using Utilities::s_clockSyncMachService;
};

class UtilitiesClockSyncTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previousService = TestableClockSyncUtilities::s_clockSyncMachService;
        TestableClockSyncUtilities::s_clockSyncMachService = "io.github.filesfm.worktime.missing-timed";
    }

    void TearDown() override { TestableClockSyncUtilities::s_clockSyncMachService = m_previousService; }

    const char *m_previousService = nullptr;
};

} // namespace

TEST_F(UtilitiesClockSyncTest, UnreachableServiceFailsOpenAsEnabled)
{
    EXPECT_TRUE(Utilities::isClockSyncEnabled());
}

#endif // defined(Q_OS_MACOS)
