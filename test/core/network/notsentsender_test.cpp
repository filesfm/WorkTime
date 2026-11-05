#include <gtest/gtest.h>

#include <utility>

#include <QFile>
#include <QFileDevice>
#include <QHostAddress>
#include <QList>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUrl>

#include "core/network/notsentsender.hpp"
#include "core/storage/sqliteconnection.hpp"

namespace {

QString databaseFilePath()
{
    class TestableSQLiteConnection : public SQLiteConnection
    {
    public:
        using SQLiteConnection::filePath;
    };
    return TestableSQLiteConnection::filePath();
}

class NotSentSenderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(databaseFilePath());
    }
};

// Waits for a connection on `server`, reads until a full HTTP request (per
// Content-Length) has arrived, then writes `response` back. Adapted from
// SendingServiceTest's captureOneRequest - simplified since these tests only
// need to control success/failure, not inspect the request body (the wire
// format itself is already covered by SendingServiceTest/ShootRequestBuilder
// tests). NotSentSender's own QNetworkAccessManager runs on its worker
// thread's nested event loop, independent of the main thread this polls on.
bool respondToNextRequest(QTcpServer &server, const QByteArray &response, int timeoutMs = 5000)
{
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
        timeoutMs);

    for (QTcpSocket *candidate : std::as_const(candidates)) {
        if (candidate != socket)
            candidate->deleteLater();
    }

    if (!complete || !socket)
        return false;

    socket->write(response);
    return socket->waitForBytesWritten(1000);
}

} // namespace

TEST_F(NotSentSenderTest, ResendsQueuedEntryOnSuccessAndEmitsSendSucceeded)
{
    {
        SQLiteConnection seed;
        seed.addEntryToNotSentTable("queued-window", 111, 222, 333);
    }

    QTcpServer server;
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost));

    NotSentSender sender;
    sender.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/remote").arg(server.serverPort())));
    QSignalSpy successSpy(&sender, &NotSentSender::sendSucceeded);
    QSignalSpy failedSpy(&sender, &NotSentSender::sendFailed);

    sender.start();
    ASSERT_TRUE(respondToNextRequest(server, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"));
    ASSERT_TRUE(successSpy.wait(5000));
    sender.stop();

    EXPECT_EQ(failedSpy.count(), 0);

    SQLiteConnection verify;
    EXPECT_EQ(verify.takeOldestEntryFromNotSentTable(), std::nullopt);
    const auto sentRow = verify.takeOldestEntryFromSentTable();
    ASSERT_TRUE(sentRow.has_value());
    EXPECT_EQ(sentRow->focusedWindowTitle, QStringLiteral("queued-window"));
    EXPECT_EQ(sentRow->utcTimestamp, 111);
}

TEST_F(NotSentSenderTest, RequeuesEntryOnFailureAndEmitsSendFailed)
{
    {
        SQLiteConnection seed;
        seed.addEntryToNotSentTable("queued-window", 111, 222, 333);
    }

    QTcpServer server;
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost));
    const quint16 port = server.serverPort();
    server.close(); // nothing listens here now, so connecting is refused

    NotSentSender sender;
    sender.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/remote").arg(port)));
    QSignalSpy successSpy(&sender, &NotSentSender::sendSucceeded);
    QSignalSpy failedSpy(&sender, &NotSentSender::sendFailed);

    sender.start();
    ASSERT_TRUE(failedSpy.wait(5000));
    sender.stop();

    EXPECT_EQ(successSpy.count(), 0);
    EXPECT_FALSE(failedSpy.at(0).at(0).toString().isEmpty());

    SQLiteConnection verify;
    EXPECT_EQ(verify.takeOldestEntryFromSentTable(), std::nullopt);
    const auto notSentRow = verify.takeOldestEntryFromNotSentTable();
    ASSERT_TRUE(notSentRow.has_value());
    EXPECT_EQ(notSentRow->focusedWindowTitle, QStringLiteral("queued-window"));
}

TEST_F(NotSentSenderTest, WakeTriggersResendOnceServerUrlIsSet)
{
    {
        SQLiteConnection seed;
        seed.addEntryToNotSentTable("queued-window", 1, 2, 3);
    }

    QTcpServer server;
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost));

    NotSentSender sender;
    QSignalSpy successSpy(&sender, &NotSentSender::sendSucceeded);

    // No server URL yet: run() finds no valid URL, leaves the queued row
    // alone, and blocks on the wake condition without a timeout.
    sender.start();
    QTest::qWait(50);
    EXPECT_EQ(successSpy.count(), 0);

    sender.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/remote").arg(server.serverPort())));

    // A single wake() can race the worker reaching its wait() call; retry
    // until the request lands.
    bool gotRequest = false;
    for (int attempt = 0; attempt < 30 && !gotRequest; ++attempt) {
        sender.wake();
        gotRequest = respondToNextRequest(server,
                                          "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                                          200);
    }
    ASSERT_TRUE(gotRequest);
    ASSERT_TRUE(successSpy.wait(5000));
    sender.stop();
}

TEST_F(NotSentSenderTest, WakeShortCircuitsTheRetryIntervalWait)
{
    {
        SQLiteConnection seed;
        seed.addEntryToNotSentTable("first", 1, 1, 1);
        seed.addEntryToNotSentTable("second", 2, 2, 2);
    }

    QTcpServer server;
    ASSERT_TRUE(server.listen(QHostAddress::LocalHost));

    NotSentSender sender;
    sender.setRetryIntervalMs(60000);
    sender.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:%1/remote").arg(server.serverPort())));
    QSignalSpy successSpy(&sender, &NotSentSender::sendSucceeded);

    sender.start();
    ASSERT_TRUE(respondToNextRequest(server, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"));
    ASSERT_TRUE(successSpy.wait(5000));
    ASSERT_EQ(successSpy.count(), 1);

    // Without wake(), the worker would sleep out the 60s retry interval
    // before picking up the second entry. A single wake() can race the
    // worker reaching its wait() call (classic missed-wakeup), so retry it
    // until the second request lands - still well inside the 60s interval,
    // proving wake() (not the timeout) is what unblocked it.
    bool gotSecondRequest = false;
    for (int attempt = 0; attempt < 30 && !gotSecondRequest; ++attempt) {
        sender.wake();
        gotSecondRequest = respondToNextRequest(server,
                                                "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
                                                200);
    }
    ASSERT_TRUE(gotSecondRequest);
    ASSERT_TRUE(successSpy.wait(5000));
    EXPECT_EQ(successSpy.count(), 2);

    sender.stop();
}

TEST_F(NotSentSenderTest, RunEndsGracefullyWhenDatabaseCannotBeOpened)
{
    QFile file(databaseFilePath());
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();
    const QFileDevice::Permissions startPermissions = file.permissions();
    ASSERT_TRUE(file.setPermissions(QFileDevice::Permissions()));

    NotSentSender sender;
    sender.setServerUrl(QUrl(QStringLiteral("http://127.0.0.1:1/remote")));
    sender.start();

    // run()'s outer catch should end the thread on its own, without ever
    // needing stop() to interrupt it.
    EXPECT_TRUE(sender.wait(5000));

    file.setPermissions(startPermissions);
}
