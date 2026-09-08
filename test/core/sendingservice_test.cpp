#include <gtest/gtest.h>

#include <utility>

#include <QByteArray>
#include <QDateTime>
#include <QHostAddress>
#include <QList>
#include <QMap>
#include <QSignalSpy>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUrl>

#include "core/sendingservice.hpp"
#include "core/settings.hpp"

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

namespace {

// The wire-format tests below run SendingService against a real local
// QTcpServer standing in for https://worktime.lv/remote, so they exercise
// the exact bytes the protocol requires - Content-Type, field names, and
// encoding - rather than internal implementation details.

struct CapturedRequest
{
    QString method;
    QString path;
    QByteArray headerBlock;
    QByteArray body;
};

CapturedRequest parseRequest(const QByteArray &raw, qint64 headerEnd)
{
    CapturedRequest request;
    const QByteArray headerBlock = headerEnd >= 0 ? raw.left(headerEnd) : raw;
    const int firstLineEnd = headerBlock.indexOf("\r\n");
    const QByteArray requestLine = firstLineEnd >= 0 ? headerBlock.left(firstLineEnd) : headerBlock;
    const QList<QByteArray> parts = requestLine.split(' ');

    request.method = parts.size() > 0 ? QString::fromLatin1(parts.at(0)) : QString();
    request.path = parts.size() > 1 ? QString::fromLatin1(parts.at(1)) : QString();
    request.headerBlock = headerBlock;
    request.body = headerEnd >= 0 ? raw.mid(headerEnd + 4) : QByteArray();
    return request;
}

// Waits for one client connection on a local server bound to serverUrl(),
// reads the full HTTP request (using Content-Length to know when the body
// is complete), replies with an empty 200 (mirroring the real endpoint),
// and waits for SendingService to report success.
//
// Reading uses QTest::qWaitFor (poll a predicate while pumping
// QCoreApplication's event loop) rather than QTcpSocket::waitForReadyRead()
// (a blocking call that does NOT pump that event loop): the client-side
// QNetworkAccessManager runs on this same thread/event loop, so a blocking
// wait here would starve it of the very loop turns it needs to actually
// write the request - it can open the TCP connection (accepted by the OS
// independently of our event loop) without ever getting to send a byte.
CapturedRequest captureOneRequest(SendingService &service)
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost)) {
        ADD_FAILURE() << "Failed to start local test server";
        return {};
    }

    service.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/remote").arg(server.serverPort())));

    QSignalSpy successSpy(&service, &SendingService::sendSucceeded);

    service.start();

    // QNetworkAccessManager may open more than one TCP connection to the
    // host (e.g. a spare connection kept ready for a future request), so
    // the first accepted connection isn't necessarily the one that ends up
    // carrying this request. Track every connection and keep whichever one
    // actually sends data.
    QList<QTcpSocket *> candidates;
    QTcpSocket *socket = nullptr;
    QByteArray raw;
    qint64 headerEnd = -1;
    qint64 expectedBodyLength = -1;

    const bool complete = QTest::qWaitFor(
        [&]() {
            while (server.hasPendingConnections())
                candidates.append(server.nextPendingConnection());

            for (QTcpSocket *candidate : std::as_const(candidates)) {
                if (candidate->bytesAvailable() <= 0)
                    continue;
                if (!socket)
                    socket = candidate;
                if (candidate == socket)
                    raw += candidate->readAll();
                else
                    candidate->readAll(); // a spare connection; drain and ignore
            }

            if (headerEnd < 0) {
                const qint64 idx = raw.indexOf("\r\n\r\n");
                if (idx >= 0) {
                    headerEnd = idx;
                    const QByteArray headerBlock = raw.left(idx);
                    for (const QByteArray &line : headerBlock.split('\n')) {
                        if (line.trimmed().toLower().startsWith("content-length:"))
                            expectedBodyLength = line.mid(line.indexOf(':') + 1).trimmed().toLongLong();
                    }
                }
            }

            if (headerEnd < 0)
                return false;
            const qint64 bodyReceived = raw.size() - (headerEnd + 4);
            return expectedBodyLength <= 0 || bodyReceived >= expectedBodyLength;
        },
        5000);

    for (QTcpSocket *candidate : std::as_const(candidates)) {
        if (candidate != socket)
            candidate->deleteLater();
    }

    if (!complete || !socket) {
        ADD_FAILURE() << "Did not receive a complete request from the client";
        service.stop();
        return {};
    }

    socket->write("HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");

    if (successSpy.isEmpty())
        successSpy.wait(5000);

    service.stop();

    return parseRequest(raw, headerEnd);
}

QMap<QString, QString> parseFormBody(const QByteArray &body)
{
    QMap<QString, QString> fields;
    for (const QByteArray &pair : body.split('&')) {
        if (pair.isEmpty())
            continue;
        const int eq = pair.indexOf('=');
        const QByteArray rawKey = eq >= 0 ? pair.left(eq) : pair;
        const QByteArray rawValue = eq >= 0 ? pair.mid(eq + 1) : QByteArray();
        fields.insert(QUrl::fromPercentEncoding(rawKey), QUrl::fromPercentEncoding(rawValue));
    }
    return fields;
}

} // namespace

TEST(SendingServiceTest, SendsFormUrlEncodedPostToConfiguredPath)
{
    Settings::instance()->setUsername("wire-format-user");
    Settings::instance()->setPassword("wire-format-pass");

    SendingService service;
    const CapturedRequest request = captureOneRequest(service);

    EXPECT_EQ(request.method, QStringLiteral("POST"));
    EXPECT_EQ(request.path, QStringLiteral("/remote"));
    EXPECT_TRUE(request.headerBlock.toLower().contains("content-type: application/x-www-form-urlencoded"));
}

TEST(SendingServiceTest, BodyContainsAllShootFieldsWithSpecifiedNames)
{
    Settings::instance()->setUsername("alice");
    Settings::instance()->setPassword("s3cret");

    SendingService service;
    const CapturedRequest request = captureOneRequest(service);
    const QMap<QString, QString> fields = parseFormBody(request.body);

    EXPECT_EQ(fields.value("Shoot[user_name]"), QStringLiteral("alice"));
    EXPECT_EQ(fields.value("Shoot[password]"), QStringLiteral("s3cret"));
    EXPECT_EQ(fields.value("Shoot[image_resized]"), QStringLiteral("1"));

    // project_name always empty: no project-tracking feature exists.
    EXPECT_EQ(fields.value("Shoot[project_name]"), QString());

    // app_name/document_name/document_path are OS-dependent (see
    // Utilities); only their presence, not their value, is guaranteed.
    for (const QString &key : {QStringLiteral("Shoot[app_name]"),
                               QStringLiteral("Shoot[document_name]"),
                               QStringLiteral("Shoot[document_path]")})
        EXPECT_TRUE(fields.contains(key)) << key.toStdString() << " missing from request body";

    EXPECT_TRUE(fields.contains(QStringLiteral("Shoot[shoot_time]")));
    EXPECT_TRUE(fields.contains(QStringLiteral("Shoot[utc_timestamp]")));
}

TEST(SendingServiceTest, UtcTimestampIsCurrentAndShootTimeMatchesLocalUtcOffset)
{
    Settings::instance()->setUsername("bob");
    Settings::instance()->setPassword("pw");

    const qint64 before = QDateTime::currentSecsSinceEpoch();

    SendingService service;
    const CapturedRequest request = captureOneRequest(service);
    const QMap<QString, QString> fields = parseFormBody(request.body);

    const qint64 after = QDateTime::currentSecsSinceEpoch();

    bool utcOk = false;
    const qint64 utcTimestamp = fields.value("Shoot[utc_timestamp]").toLongLong(&utcOk);
    ASSERT_TRUE(utcOk);
    EXPECT_GE(utcTimestamp, before - 5);
    EXPECT_LE(utcTimestamp, after + 5);

    bool shootOk = false;
    const qint64 shootTime = fields.value("Shoot[shoot_time]").toLongLong(&shootOk);
    ASSERT_TRUE(shootOk);
    EXPECT_EQ(shootTime - utcTimestamp, QDateTime::currentDateTime().offsetFromUtc());
}

// Credentials must be sent exactly as configured - not trimmed and not
// truncated to the 255-character metadata limit.
TEST(SendingServiceTest, CredentialsArePassedThroughUnmodified)
{
    const QString password = QStringLiteral(" ") + QString(300, QLatin1Char('p')) + QStringLiteral(" ");
    Settings::instance()->setUsername("carol");
    Settings::instance()->setPassword(password);

    SendingService service;
    const CapturedRequest request = captureOneRequest(service);
    const QMap<QString, QString> fields = parseFormBody(request.body);

    EXPECT_EQ(fields.value("Shoot[password]"), password);
}
