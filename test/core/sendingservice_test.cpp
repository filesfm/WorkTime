#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>
#include <QUrl>

#include "core/sendingservice.hpp"

TEST(SendingServiceTest, DefaultServerUrlIsInvalid)
{
    SendingService service;
    EXPECT_FALSE(service.serverUrl().isValid());
}

TEST(SendingServiceTest, ServerUrlRoundTrips)
{
    SendingService service;
    const QUrl url("https://example.com/api");
    service.setServerUrl(url);
    EXPECT_EQ(service.serverUrl(), url);
}

TEST(SendingServiceTest, IsActiveReflectsStartAndStop)
{
    SendingService service;
    EXPECT_FALSE(service.isActive());

    service.start();
    EXPECT_TRUE(service.isActive());

    service.stop();
    EXPECT_FALSE(service.isActive());
}

// With no server URL configured, sendNow() (triggered by start()) must fail
// fast rather than attempting a network request.
TEST(SendingServiceTest, StartWithoutServerUrlEmitsSendFailed)
{
    SendingService service;
    QSignalSpy failedSpy(&service, &SendingService::sendFailed);

    service.start();
    service.stop();

    ASSERT_EQ(failedSpy.count(), 1);
    EXPECT_EQ(failedSpy.at(0).at(0).toString(), QStringLiteral("No server URL configured"));
}

TEST(SendingServiceTest, StopWithoutStartIsSafe)
{
    SendingService service;
    EXPECT_FALSE(service.isActive());

    service.stop();
    EXPECT_FALSE(service.isActive());
}

// A valid server URL takes the request-posting branch of sendNow() instead
// of the fail-fast one; the request itself is left in flight (no event loop
// is spun here), so this only checks that "No server URL configured" isn't
// emitted synchronously.
TEST(SendingServiceTest, StartWithValidServerUrlDoesNotFailFast)
{
    SendingService service;
    service.setServerUrl(QUrl("https://example.com/api"));
    QSignalSpy failedSpy(&service, &SendingService::sendFailed);

    service.start();
    service.stop();

    EXPECT_EQ(failedSpy.count(), 0);
}

TEST(SendingServiceTest, RestartTriggersAnotherImmediateSend)
{
    SendingService service;
    QSignalSpy failedSpy(&service, &SendingService::sendFailed);

    service.start();
    service.stop();
    service.start();
    service.stop();

    EXPECT_EQ(failedSpy.count(), 2);
}
