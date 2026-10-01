#include <gtest/gtest.h>

#include "core/activity/activitymonitor.hpp"

#if defined(Q_OS_LINUX)
#    include <QByteArray>
#    include <QDBusConnection>
#    include <QDBusConnectionInterface>
#endif

namespace {

class TestableActivityMonitor : public ActivityMonitor
{
public:
    bool isRunning() const { return m_running; }

#if defined(Q_OS_LINUX)
    using ActivityMonitor::startGnomeIdleMonitor;
    using ActivityMonitor::stopGnomeIdleMonitor;

    bool isUsingGnomeIdleMonitor() const { return m_usingGnomeIdleMonitor; }
    bool hasGnomeIdleMonitorInterface() const { return m_gnomeIdleMonitor != nullptr; }
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