#include <gtest/gtest.h>

#include "core/activity/activitymonitor.hpp"

#if defined(Q_OS_LINUX)
#    include <QByteArray>
#    include <QDBusConnection>
#    include <QDBusConnectionInterface>
#    include <QDBusError>
#    include <QDBusMessage>
#    include <QDBusVirtualObject>
#    include <QFile>
#    include <QList>
#    include <QSignalSpy>
#    include <QString>
#    include <QTemporaryDir>
#    include <QTest>

#    include <fcntl.h>
#    include <linux/input.h>
#    include <sys/stat.h>
#    include <unistd.h>
#endif

namespace {

class TestableActivityMonitor : public ActivityMonitor
{
public:
    bool isRunning() const { return m_running; }

#if defined(Q_OS_LINUX)
    using ActivityMonitor::addInputDevice;
    using ActivityMonitor::readInputDevice;
    using ActivityMonitor::removeInputDevice;
    using ActivityMonitor::s_gnomeIdleMonitorService;
    using ActivityMonitor::startGnomeIdleMonitor;
    using ActivityMonitor::stopGnomeIdleMonitor;

    bool isUsingGnomeIdleMonitor() const { return m_usingGnomeIdleMonitor; }
    bool hasGnomeIdleMonitorInterface() const { return m_gnomeIdleMonitor != nullptr; }
    int deviceNotifierCount() const { return m_deviceNotifiers.size(); }
#endif
};

} // namespace

TEST(ActivityMonitorTest, ConstructingDoesNotStartMonitoring)
{
    TestableActivityMonitor monitor;
    EXPECT_FALSE(monitor.isRunning());
}

TEST(ActivityMonitorTest, StartSetsRunningState)
{
    TestableActivityMonitor monitor;
    monitor.start();

    EXPECT_TRUE(monitor.isRunning());

    monitor.stop();
}

TEST(ActivityMonitorTest, StopWithoutStartIsNoOp)
{
    TestableActivityMonitor monitor;

    EXPECT_NO_THROW(monitor.stop());
    EXPECT_FALSE(monitor.isRunning());
}

TEST(ActivityMonitorTest, StartIsIdempotent)
{
    TestableActivityMonitor monitor;
    monitor.start();

    EXPECT_NO_THROW(monitor.start());
    EXPECT_TRUE(monitor.isRunning());

    monitor.stop();
}

TEST(ActivityMonitorTest, StopIsIdempotent)
{
    TestableActivityMonitor monitor;
    monitor.start();
    monitor.stop();

    EXPECT_NO_THROW(monitor.stop());
    EXPECT_FALSE(monitor.isRunning());
}

TEST(ActivityMonitorTest, SupportsRestartAfterStop)
{
    TestableActivityMonitor monitor;
    monitor.start();
    monitor.stop();
    monitor.start();

    EXPECT_TRUE(monitor.isRunning());

    monitor.stop();
}

TEST(ActivityMonitorTest, DestructionWhileRunningDoesNotCrash)
{
    {
        TestableActivityMonitor monitor;
        monitor.start();
    }
    SUCCEED();
}

#if defined(Q_OS_LINUX)

namespace {

// Stands in for org.gnome.Mutter.IdleMonitor's /Core object. Declares
// AddUserActiveWatch/RemoveWatch in its introspection XML - QDBusInterface's
// isValid() (which startGnomeIdleMonitor() gates on) requires that, same
// lesson as CredentialStoreTest's unlock-prompt fake. WatchFired itself is
// subscribed to via the low-level QDBusConnection::connect() API, which
// doesn't need introspection, so it isn't declared here.
class FakeGnomeIdleMonitor : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;
    uint nextWatchId = 42;
    bool addWatchShouldFail = false;

    QString introspect(const QString &) const override
    {
        return QStringLiteral("<interface name=\"org.gnome.Mutter.IdleMonitor\">"
                              "<method name=\"AddUserActiveWatch\"><arg type=\"u\" direction=\"out\"/></method>"
                              "<method name=\"RemoveWatch\"><arg type=\"u\" direction=\"in\"/></method>"
                              "</interface>");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("AddUserActiveWatch")) {
            if (addWatchShouldFail)
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no watch")));
            else
                connection.send(message.createReply(QVariant::fromValue(nextWatchId)));
        } else if (message.member() == QStringLiteral("RemoveWatch")) {
            connection.send(message.createReply());
        }
        return true;
    }

    void fireWatch(QDBusConnection &connection, uint watchId)
    {
        QDBusMessage signal = QDBusMessage::createSignal(QStringLiteral("/org/gnome/Mutter/IdleMonitor/Core"),
                                                         QStringLiteral("org.gnome.Mutter.IdleMonitor"),
                                                         QStringLiteral("WatchFired"));
        signal << watchId;
        connection.send(signal);
    }
};

// Serves FakeGnomeIdleMonitor on this process's own session-bus connection
// and points startGnomeIdleMonitor() at it, and sets XDG_CURRENT_DESKTOP so
// that branch is actually taken - same technique used throughout this test
// suite for D-Bus-backed classes.
class ActivityMonitorGnomeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/gnome/Mutter/IdleMonitor/Core", &m_idleMonitor));
        m_previousService = TestableActivityMonitor::s_gnomeIdleMonitorService;
        TestableActivityMonitor::s_gnomeIdleMonitorService = m_bus.baseService();
        m_previousDesktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
        qputenv("XDG_CURRENT_DESKTOP", "gnome");
    }

    void TearDown() override
    {
        qputenv("XDG_CURRENT_DESKTOP", m_previousDesktop.toUtf8());
        if (!m_bus.isConnected())
            return;

        TestableActivityMonitor::s_gnomeIdleMonitorService = m_previousService;
        m_bus.unregisterObject("/org/gnome/Mutter/IdleMonitor/Core");
    }

    int addUserActiveWatchCallCount() const
    {
        int count = 0;
        for (const QDBusMessage &call : m_idleMonitor.calls) {
            if (call.member() == QStringLiteral("AddUserActiveWatch"))
                ++count;
        }
        return count;
    }

    QDBusConnection m_bus{QString()};
    FakeGnomeIdleMonitor m_idleMonitor;
    QString m_previousService;
    QString m_previousDesktop;
};

} // namespace

TEST_F(ActivityMonitorGnomeTest, StartUsesGnomeIdleMonitorUnderGnome)
{
    TestableActivityMonitor monitor;
    monitor.start();

    EXPECT_TRUE(monitor.isUsingGnomeIdleMonitor());
    EXPECT_TRUE(monitor.hasGnomeIdleMonitorInterface());
    EXPECT_TRUE(monitor.isRunning());
    EXPECT_EQ(addUserActiveWatchCallCount(), 1);

    monitor.stop();
}

TEST_F(ActivityMonitorGnomeTest, StopRemovesTheWatchAndDropsTheInterface)
{
    TestableActivityMonitor monitor;
    monitor.start();
    monitor.stop();

    bool sawRemoveWatch = false;
    for (const QDBusMessage &call : m_idleMonitor.calls) {
        if (call.member() == QStringLiteral("RemoveWatch"))
            sawRemoveWatch = true;
    }
    EXPECT_TRUE(sawRemoveWatch);
    EXPECT_FALSE(monitor.isRunning());
    EXPECT_FALSE(monitor.hasGnomeIdleMonitorInterface());
}

TEST_F(ActivityMonitorGnomeTest, WatchFiredEmitsActivityDetectedAndRearms)
{
    TestableActivityMonitor monitor;
    monitor.start();
    ASSERT_EQ(addUserActiveWatchCallCount(), 1);

    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    m_idleMonitor.fireWatch(m_bus, m_idleMonitor.nextWatchId);

    ASSERT_TRUE(spy.wait(2000));
    EXPECT_EQ(spy.count(), 1);
    // AddUserActiveWatch is one-shot: a fired watch must be re-added.
    EXPECT_EQ(addUserActiveWatchCallCount(), 2);

    monitor.stop();
}

TEST_F(ActivityMonitorGnomeTest, WatchFiredWithStaleIdIsIgnored)
{
    TestableActivityMonitor monitor;
    monitor.start();

    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    m_idleMonitor.fireWatch(m_bus, m_idleMonitor.nextWatchId + 999); // not the id AddUserActiveWatch replied with

    EXPECT_FALSE(spy.wait(200));
    EXPECT_EQ(spy.count(), 0);

    monitor.stop();
}

TEST_F(ActivityMonitorGnomeTest, FallsBackWhenGnomeIdleMonitorInterfaceIsUnavailable)
{
    // Overrides the fixture's own (working) override with a destination
    // nothing answers, so QDBusInterface::isValid() is false.
    TestableActivityMonitor::s_gnomeIdleMonitorService = QStringLiteral("org.nonexistent.worktime.test.gnome");

    TestableActivityMonitor monitor;
    monitor.start();

    EXPECT_FALSE(monitor.isUsingGnomeIdleMonitor());
    EXPECT_FALSE(monitor.hasGnomeIdleMonitorInterface());
    EXPECT_TRUE(monitor.isRunning()); // falls through to the udev fallback

    monitor.stop();
}

TEST_F(ActivityMonitorGnomeTest, AddUserActiveWatchFailureIsNonFatal)
{
    m_idleMonitor.addWatchShouldFail = true;

    TestableActivityMonitor monitor;
    monitor.start();

    // Still "using" the GNOME monitor - the interface itself is fine, only
    // the watch request failed - but no watch id is tracked.
    EXPECT_TRUE(monitor.isUsingGnomeIdleMonitor());
    EXPECT_TRUE(monitor.isRunning());

    monitor.stop();
}

// The udev-based fallback's enumeration/monitor layer (scanInputDevices(),
// readUdevMonitor()) isn't realistically fakeable without intercepting real
// libudev calls, but addInputDevice()/removeInputDevice()/readInputDevice()
// only need an openable path or a plain fd - a FIFO and a pipe stand in for
// a /dev/input device node without needing udev at all.
class ActivityMonitorInputDeviceTest : public ::testing::Test
{
protected:
    void TearDown() override
    {
        for (const QString &path : m_fifoPaths)
            QFile::remove(path);
    }

    QString makeFifo()
    {
        if (!m_tempDir.isValid())
            return QString();
        const QString path = m_tempDir.filePath(QStringLiteral("fake-input-device-%1").arg(m_fifoPaths.size()));
        if (::mkfifo(path.toUtf8().constData(), 0600) != 0)
            return QString();
        m_fifoPaths.append(path);
        return path;
    }

    QTemporaryDir m_tempDir;
    QList<QString> m_fifoPaths;
};

TEST_F(ActivityMonitorInputDeviceTest, AddInputDeviceTracksAnOpenablePath)
{
    const QString fifo = makeFifo();
    ASSERT_FALSE(fifo.isEmpty());

    TestableActivityMonitor monitor;
    monitor.addInputDevice(fifo);

    EXPECT_EQ(monitor.deviceNotifierCount(), 1);
}

TEST_F(ActivityMonitorInputDeviceTest, AddInputDeviceIsNoOpForAnAlreadyTrackedPath)
{
    const QString fifo = makeFifo();
    ASSERT_FALSE(fifo.isEmpty());

    TestableActivityMonitor monitor;
    monitor.addInputDevice(fifo);
    monitor.addInputDevice(fifo);

    EXPECT_EQ(monitor.deviceNotifierCount(), 1);
}

TEST_F(ActivityMonitorInputDeviceTest, AddInputDeviceIgnoresAPathThatCannotBeOpened)
{
    TestableActivityMonitor monitor;
    monitor.addInputDevice(QStringLiteral("/nonexistent/worktime-test-device"));

    EXPECT_EQ(monitor.deviceNotifierCount(), 0);
}

TEST_F(ActivityMonitorInputDeviceTest, RemoveInputDeviceDropsATrackedPath)
{
    const QString fifo = makeFifo();
    ASSERT_FALSE(fifo.isEmpty());

    TestableActivityMonitor monitor;
    monitor.addInputDevice(fifo);
    monitor.removeInputDevice(fifo);

    EXPECT_EQ(monitor.deviceNotifierCount(), 0);
}

TEST_F(ActivityMonitorInputDeviceTest, RemoveInputDeviceIsNoOpForAnUntrackedPath)
{
    TestableActivityMonitor monitor;

    EXPECT_NO_THROW(monitor.removeInputDevice(QStringLiteral("/nonexistent/worktime-test-device")));
    EXPECT_EQ(monitor.deviceNotifierCount(), 0);
}

namespace {

// A plain pipe stands in for an input device fd: readInputDevice() only
// ever does raw read()s on whatever fd it's given.
class ActivityMonitorReadInputDeviceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(::pipe(m_fds), 0);
        ASSERT_EQ(::fcntl(m_fds[0], F_SETFL, O_NONBLOCK), 0);
    }

    void TearDown() override
    {
        ::close(m_fds[0]);
        ::close(m_fds[1]);
    }

    void writeEvent(quint16 type)
    {
        struct input_event event{};
        event.type = type;
        ASSERT_EQ(::write(m_fds[1], &event, sizeof(event)), static_cast<ssize_t>(sizeof(event)));
    }

    int m_fds[2] = {-1, -1};
};

} // namespace

TEST_F(ActivityMonitorReadInputDeviceTest, KeyEventEmitsActivityDetected)
{
    writeEvent(EV_KEY);

    TestableActivityMonitor monitor;
    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    monitor.readInputDevice(m_fds[0]);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ActivityMonitorReadInputDeviceTest, RelEventEmitsActivityDetected)
{
    writeEvent(EV_REL);

    TestableActivityMonitor monitor;
    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    monitor.readInputDevice(m_fds[0]);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ActivityMonitorReadInputDeviceTest, AbsEventEmitsActivityDetected)
{
    writeEvent(EV_ABS);

    TestableActivityMonitor monitor;
    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    monitor.readInputDevice(m_fds[0]);

    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ActivityMonitorReadInputDeviceTest, OtherEventTypeDoesNotEmitActivityDetected)
{
    writeEvent(EV_SYN);

    TestableActivityMonitor monitor;
    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    monitor.readInputDevice(m_fds[0]);

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ActivityMonitorReadInputDeviceTest, NoDataAvailableDoesNotEmitActivityDetected)
{
    TestableActivityMonitor monitor;
    QSignalSpy spy(&monitor, &ActivityMonitor::activityDetected);
    monitor.readInputDevice(m_fds[0]);

    EXPECT_EQ(spy.count(), 0);
}

#endif // defined(Q_OS_LINUX)