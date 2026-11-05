#include <gtest/gtest.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVirtualObject>
#include <QList>
#include <QString>

#include "core/kwinscript.hpp"

namespace {

class TestableKWinScript : public KWinScript
{
public:
    using KWinScript::s_serviceName;
};

// Stands in for org.kde.KWin's /Scripting object: tracks every call it
// receives and replies the way the real KWin does.
class FakeKWinScripting : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    bool reportAlreadyLoaded = false;
    int loadScriptReturnValue = 7;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        // Qt also routes org.freedesktop.DBus.Introspectable.Introspect here;
        // let it fall through to introspect() instead of counting it as a call.
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        calls.append(message);

        if (message.member() == QStringLiteral("isScriptLoaded")) {
            connection.send(message.createReply(reportAlreadyLoaded));
        } else if (message.member() == QStringLiteral("loadScript")) {
            connection.send(message.createReply(loadScriptReturnValue));
        } else if (message.member() == QStringLiteral("unloadScript")) {
            connection.send(message.createReply());
        }
        return true;
    }
};

// Stands in for a loaded script's own /Scripting/Script<id> object, just
// enough to answer the "run" call KWinScript::load() makes after loading.
class FakeKWinScriptObject : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        calls.append(message);
        connection.send(message.createReply());
        return true;
    }
};

// Serves the two fakes above on this process's own session-bus connection
// and points KWinScript at it, so tests never talk to a real KWin - same
// technique as UtilitiesAutostartTest for the Background portal.
class KWinScriptTest : public ::testing::Test
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
        if (!m_bus.isConnected())
            return;

        TestableKWinScript::s_serviceName = m_previousService;
        m_bus.unregisterObject("/Scripting");
        m_bus.unregisterObject("/Scripting/Script7");
    }

    QDBusConnection m_bus{QString()};
    FakeKWinScripting m_scripting;
    FakeKWinScriptObject m_scriptObject;
    QString m_previousService;
};

} // namespace

TEST_F(KWinScriptTest, LoadReturnsTrueAndRunsTheScriptWhenNotAlreadyLoaded)
{
    EXPECT_TRUE(KWinScript::load("worktime-test", ":/some/resource.js", "worktime-test.js"));

    bool sawLoadScript = false;
    for (const QDBusMessage &call : m_scripting.calls) {
        if (call.member() == QStringLiteral("loadScript"))
            sawLoadScript = true;
    }
    EXPECT_TRUE(sawLoadScript);
    EXPECT_EQ(m_scriptObject.calls.size(), 1);
    EXPECT_EQ(m_scriptObject.calls.constFirst().member(), QStringLiteral("run"));
}

TEST_F(KWinScriptTest, LoadUnloadsAnExistingCopyFirstWhenAlreadyLoaded)
{
    m_scripting.reportAlreadyLoaded = true;

    EXPECT_TRUE(KWinScript::load("worktime-test", ":/some/resource.js", "worktime-test.js"));

    bool sawUnloadScript = false;
    for (const QDBusMessage &call : m_scripting.calls) {
        if (call.member() == QStringLiteral("unloadScript"))
            sawUnloadScript = true;
    }
    EXPECT_TRUE(sawUnloadScript);
}

TEST_F(KWinScriptTest, UnloadCallsUnloadScript)
{
    KWinScript::unload("worktime-test");

    ASSERT_EQ(m_scripting.calls.size(), 1);
    EXPECT_EQ(m_scripting.calls.constFirst().member(), QStringLiteral("unloadScript"));
}

TEST_F(KWinScriptTest, LoadReturnsFalseWhenScriptingInterfaceIsUnavailable)
{
    TestableKWinScript::s_serviceName = QStringLiteral("org.nonexistent.worktime.test.kwin");

    EXPECT_FALSE(KWinScript::load("worktime-test", ":/some/resource.js", "worktime-test.js"));
    EXPECT_TRUE(m_scripting.calls.isEmpty());
}
