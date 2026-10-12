#include "betterstacklogger.hpp"

#include "betterstacklogentrybuilder.hpp"
#include "core/logging/betterstackconfig.hpp"

#include <QDateTime>
#include <QEventLoop>
#include <QMessageLogContext>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <cstdio>
#include <cstring>

Q_LOGGING_CATEGORY(worktimeBetterStackLogger, "worktime.betterstack.logger")

void BetterStackLogger::install()
{
    if (std::strlen(WORKTIME_BETTERSTACK_SOURCE_TOKEN) == 0 || std::strlen(WORKTIME_BETTERSTACK_INGESTING_HOST) == 0)
        return;

    qInstallMessageHandler(&BetterStackLogger::messageHandler);
}

void BetterStackLogger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    if (qEnvironmentVariableIsSet("QT_LOGGING_RULES")) {
        const QString formatted = qFormatLogMessage(type, context, message);
        std::fprintf(stderr, "%s\n", qPrintable(formatted));
        std::fflush(stderr);
    }

    if (type != QtCriticalMsg && type != QtFatalMsg)
        return;

    send(type, message, QString::fromUtf8(context.category ? context.category : "default"));
}

void BetterStackLogger::send(QtMsgType type, const QString &message, const QString &category)
{
    // Guards against a critical/fatal message raised by the POST below (e.g.
    // from Qt's network/SSL internals) recursing back into this function.
    static thread_local bool sending = false;
    if (sending)
        return;
    sending = true;

    const QByteArray body = buildBetterStackLogEntry(type,
                                                     message,
                                                     category,
                                                     QDateTime::currentMSecsSinceEpoch(),
                                                     QString::fromUtf8(WORKTIME_APP_VERSION));

    QNetworkAccessManager manager;
    QNetworkRequest request(
        QUrl(QStringLiteral("https://%1").arg(QString::fromUtf8(WORKTIME_BETTERSTACK_INGESTING_HOST))));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setRawHeader("Authorization", QByteArray("Bearer ") + WORKTIME_BETTERSTACK_SOURCE_TOKEN);
    // Short and blocking: a QtFatalMsg aborts the process as soon as this
    // function returns, so delivery must be attempted synchronously rather
    // than queued for later on an event loop that may never run again.
    request.setTransferTimeout(5000);

    QEventLoop loop;
    QNetworkReply *reply = manager.post(request, body);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError)
        qCWarning(worktimeBetterStackLogger) << "failed to forward log entry to Better Stack:" << reply->errorString();

    reply->deleteLater();

    sending = false;
}
