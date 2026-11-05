#include <gtest/gtest.h>

#include <QHostAddress>
#include <QMessageLogContext>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include "core/logging/betterstacklogger.hpp"

namespace {

class TestableBetterStackLogger : public BetterStackLogger
{
public:
    using BetterStackLogger::messageHandler;
    using BetterStackLogger::s_ingestingHost;
    using BetterStackLogger::s_sourceToken;
};

// Points send()'s destination at a local TCP listener and reports whether a
// connection attempt arrived. The listener never speaks TLS, so send()'s own
// HTTPS request would otherwise sit until its 5s transfer timeout - aborting
// the connection as soon as it arrives (while send()'s own nested event loop
// is still spinning) makes the client fail fast instead.
class BetterStackLoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(m_server.listen(QHostAddress::LocalHost));
        QObject::connect(&m_server, &QTcpServer::newConnection, [this]() {
            m_sawConnection = true;
            while (m_server.hasPendingConnections()) {
                QTcpSocket *socket = m_server.nextPendingConnection();
                socket->abort();
                socket->deleteLater();
            }
        });

        m_previousHost = TestableBetterStackLogger::s_ingestingHost;
        m_previousToken = TestableBetterStackLogger::s_sourceToken;
        TestableBetterStackLogger::s_ingestingHost = QStringLiteral("127.0.0.1:%1").arg(m_server.serverPort());
        TestableBetterStackLogger::s_sourceToken = QStringLiteral("test-token");
    }

    void TearDown() override
    {
        TestableBetterStackLogger::s_ingestingHost = m_previousHost;
        TestableBetterStackLogger::s_sourceToken = m_previousToken;
    }

    QTcpServer m_server;
    bool m_sawConnection = false;
    QString m_previousHost;
    QString m_previousToken;
};

} // namespace

TEST_F(BetterStackLoggerTest, DebugMessagesAreNotForwarded)
{
    QMessageLogContext context;
    TestableBetterStackLogger::messageHandler(QtDebugMsg, context, QStringLiteral("debug"));

    EXPECT_FALSE(m_sawConnection);
}

TEST_F(BetterStackLoggerTest, InfoMessagesAreNotForwarded)
{
    QMessageLogContext context;
    TestableBetterStackLogger::messageHandler(QtInfoMsg, context, QStringLiteral("info"));

    EXPECT_FALSE(m_sawConnection);
}

TEST_F(BetterStackLoggerTest, WarningMessagesAreNotForwarded)
{
    QMessageLogContext context;
    TestableBetterStackLogger::messageHandler(QtWarningMsg, context, QStringLiteral("warning"));

    EXPECT_FALSE(m_sawConnection);
}

TEST_F(BetterStackLoggerTest, CriticalMessagesAreForwarded)
{
    QMessageLogContext context;
    TestableBetterStackLogger::messageHandler(QtCriticalMsg, context, QStringLiteral("critical"));

    EXPECT_TRUE(m_sawConnection);
}

TEST_F(BetterStackLoggerTest, FatalMessagesAreForwarded)
{
    // messageHandler() itself never aborts the process - only Qt's own
    // qt_message_output() does, after the installed handler returns - so
    // calling it directly with QtFatalMsg here is safe.
    QMessageLogContext context;
    TestableBetterStackLogger::messageHandler(QtFatalMsg, context, QStringLiteral("fatal"));

    EXPECT_TRUE(m_sawConnection);
}

TEST_F(BetterStackLoggerTest, NullCategoryDefaultsToDefaultCategory)
{
    QMessageLogContext context(nullptr, 0, nullptr, nullptr);
    TestableBetterStackLogger::messageHandler(QtCriticalMsg, context, QStringLiteral("critical"));

    EXPECT_TRUE(m_sawConnection);
}

namespace {

// install() only ever changes Qt's process-wide message handler, so every
// test here restores the default handler in TearDown() regardless of
// outcome, to avoid leaking into whichever test runs next.
class BetterStackLoggerInstallTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_previousHost = TestableBetterStackLogger::s_ingestingHost;
        m_previousToken = TestableBetterStackLogger::s_sourceToken;
    }

    void TearDown() override
    {
        TestableBetterStackLogger::s_ingestingHost = m_previousHost;
        TestableBetterStackLogger::s_sourceToken = m_previousToken;
        qInstallMessageHandler(nullptr);
    }

    QString m_previousHost;
    QString m_previousToken;
};

} // namespace

TEST_F(BetterStackLoggerInstallTest, IsNoOpWhenTokenIsEmpty)
{
    TestableBetterStackLogger::s_ingestingHost = QStringLiteral("example.invalid");
    TestableBetterStackLogger::s_sourceToken = QString();

    const QtMessageHandler marker = [](QtMsgType, const QMessageLogContext &, const QString &) {};
    qInstallMessageHandler(marker);

    BetterStackLogger::install();

    EXPECT_EQ(qInstallMessageHandler(marker), marker);
}

TEST_F(BetterStackLoggerInstallTest, IsNoOpWhenHostIsEmpty)
{
    TestableBetterStackLogger::s_ingestingHost = QString();
    TestableBetterStackLogger::s_sourceToken = QStringLiteral("token");

    const QtMessageHandler marker = [](QtMsgType, const QMessageLogContext &, const QString &) {};
    qInstallMessageHandler(marker);

    BetterStackLogger::install();

    EXPECT_EQ(qInstallMessageHandler(marker), marker);
}

TEST_F(BetterStackLoggerInstallTest, InstallsTheHandlerWhenBothAreConfigured)
{
    TestableBetterStackLogger::s_ingestingHost = QStringLiteral("example.invalid");
    TestableBetterStackLogger::s_sourceToken = QStringLiteral("token");

    const QtMessageHandler marker = [](QtMsgType, const QMessageLogContext &, const QString &) {};
    qInstallMessageHandler(marker);

    BetterStackLogger::install();

    EXPECT_EQ(qInstallMessageHandler(marker), &TestableBetterStackLogger::messageHandler);
}
