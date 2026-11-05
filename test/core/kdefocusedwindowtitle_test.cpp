#include <gtest/gtest.h>
#include <type_traits>

#include "core/kdefocusedwindowtitle.hpp"

#if defined(Q_OS_LINUX)
#    include <QDBusConnection>
#    include <QDBusConnectionInterface>
#    include <QDBusMessage>
#    include <QDBusVirtualObject>
#    include <QList>
#    include <QString>

#    include "core/kwinscript.hpp"
#endif

TEST(KDEFocusedWindowTitleTest, IsNotCopyableOrMovable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_copy_assignable_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_move_constructible_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_move_assignable_v<KDEFocusedWindowTitle>);
}

TEST(KDEFocusedWindowTitleTest, InstanceReturnsNonNullPointer)
{
    EXPECT_NE(KDEFocusedWindowTitle::instance(), nullptr);
}

TEST(KDEFocusedWindowTitleTest, InstanceReturnsSameInstanceOnRepeatedCalls)
{
    EXPECT_EQ(KDEFocusedWindowTitle::instance(), KDEFocusedWindowTitle::instance());
}

// This must run before any test that calls setActiveWindowTitle().
TEST(KDEFocusedWindowTitleTest, ActiveWindowTitleIsEmptyBeforeAnyTitleIsSet)
{
    EXPECT_TRUE(KDEFocusedWindowTitle::instance()->activeWindowTitle().isEmpty());
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleUpdatesActiveWindowTitle)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Editor - file.cpp"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("Editor - file.cpp"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleOverwritesPreviousValue)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("First Window"));
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Second Window"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("Second Window"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleAcceptsEmptyString)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Some Window"));
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QString());

    EXPECT_TRUE(KDEFocusedWindowTitle::instance()->activeWindowTitle().isEmpty());
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleHandlesUnicodeCharacters)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("日本語 — emoji 🎉 window"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("日本語 — emoji 🎉 window"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleDoesNotTrimWhitespace)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("  padded title  "));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("  padded title  "));
}

#if defined(Q_OS_LINUX)

namespace {

constexpr auto kServiceName = "io.github.filesfm.worktime";

class TestableKWinScript : public KWinScript
{
public:
    using KWinScript::s_serviceName;
};

// Stands in for org.kde.KWin's /Scripting object - same fake as
// KWinScriptTest, extended with a per-member call counter and a controllable
// loadScript failure, since start()'s own behavior hinges on both.
class FakeKWinScripting : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool loadScriptShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("isScriptLoaded")) {
            connection.send(message.createReply(false));
        } else if (message.member() == QStringLiteral("loadScript")) {
            connection.send(message.createReply(loadScriptShouldFail ? -1 : 7));
        } else if (message.member() == QStringLiteral("unloadScript")) {
            connection.send(message.createReply());
        }
        return true;
    }

    int callCountFor(const QString &member) const
    {
        int count = 0;
        for (const QDBusMessage &call : calls) {
            if (call.member() == member)
                ++count;
        }
        return count;
    }
};

class FakeKWinScriptObject : public QDBusVirtualObject
{
public:
    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        connection.send(message.createReply());
        return true;
    }
};

// KDEFocusedWindowTitle::instance() is a process-wide singleton, so every
// test here must leave it fully stopped (and the service name released)
// before the next test runs, regardless of what the test itself did.
class KDEFocusedWindowTitleStartStopTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/Scripting", &m_scripting));
        ASSERT_TRUE(m_bus.registerVirtualObject("/Scripting/Script7", &m_scriptObject));
        m_previousService = TestableKWinScript::s_serviceName;
        TestableKWinScript::s_serviceName = m_bus.baseService();
    }

    void TearDown() override
    {
        KDEFocusedWindowTitle::instance()->stop();
        if (!m_bus.isConnected())
            return;

        // Belt-and-suspenders: stop() only unregisters the service if
        // m_started was true, which it never is after a failed start(), so
        // a failed-load test would otherwise leak the registration.
        m_bus.unregisterService(QString::fromLatin1(kServiceName));

        TestableKWinScript::s_serviceName = m_previousService;
        m_bus.unregisterObject("/Scripting");
        m_bus.unregisterObject("/Scripting/Script7");
    }

    bool isServiceRegistered() const
    {
        return m_bus.interface()->isServiceRegistered(QString::fromLatin1(kServiceName));
    }

    QDBusConnection m_bus{QString()};
    FakeKWinScripting m_scripting;
    FakeKWinScriptObject m_scriptObject;
    QString m_previousService;
};

} // namespace

TEST_F(KDEFocusedWindowTitleStartStopTest, StartRegistersServiceAndLoadsScript)
{
    KDEFocusedWindowTitle::instance()->start();

    EXPECT_TRUE(isServiceRegistered());
    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("loadScript")), 1);
}

TEST_F(KDEFocusedWindowTitleStartStopTest, StartIsNoOpWhenAlreadyStarted)
{
    KDEFocusedWindowTitle::instance()->start();
    KDEFocusedWindowTitle::instance()->start();

    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("loadScript")), 1);
}

TEST_F(KDEFocusedWindowTitleStartStopTest, StopUnloadsScriptAndUnregistersService)
{
    KDEFocusedWindowTitle::instance()->start();
    KDEFocusedWindowTitle::instance()->stop();

    EXPECT_FALSE(isServiceRegistered());
    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("unloadScript")), 1);
}

TEST_F(KDEFocusedWindowTitleStartStopTest, StopIsNoOpWhenNotStarted)
{
    KDEFocusedWindowTitle::instance()->stop();

    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("unloadScript")), 0);
}

TEST_F(KDEFocusedWindowTitleStartStopTest, StartLogsCriticalWhenServiceRegistrationFails)
{
    // A second connection claims the well-known name first, so start()'s
    // own registerService() call on the default session bus fails.
    QDBusConnection blocker = QDBusConnection::connectToBus(QDBusConnection::SessionBus,
                                                            QStringLiteral("worktime-test-blocker"));
    ASSERT_TRUE(blocker.isConnected());
    ASSERT_TRUE(blocker.registerService(QString::fromLatin1(kServiceName)));

    KDEFocusedWindowTitle::instance()->start();

    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("loadScript")), 0);

    blocker.unregisterService(QString::fromLatin1(kServiceName));
    QDBusConnection::disconnectFromBus(QStringLiteral("worktime-test-blocker"));
}

TEST_F(KDEFocusedWindowTitleStartStopTest, StartDoesNotMarkStartedWhenScriptLoadFails)
{
    m_scripting.loadScriptShouldFail = true;

    KDEFocusedWindowTitle::instance()->start();
    // If the first attempt had set m_started, this second call would be a
    // no-op and loadScript would not be attempted again.
    KDEFocusedWindowTitle::instance()->start();

    EXPECT_EQ(m_scripting.callCountFor(QStringLiteral("loadScript")), 2);
}

#endif // defined(Q_OS_LINUX)
