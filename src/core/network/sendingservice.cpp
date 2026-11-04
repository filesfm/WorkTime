#include "sendingservice.hpp"

#include "core/network/shootrequestbuilder.hpp"
#include "core/settings/settings.hpp"
#include "core/utilities.hpp"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <exception>

Q_LOGGING_CATEGORY(worktimeSendingService, "worktime.sending.service")

namespace {
constexpr int kNormalIntervalMs = 60000;
// Fast poll while blocked on a disabled OS clock sync, so sending resumes
// near-instantly once the user re-enables it, instead of waiting for the
// next normal-cadence tick.
constexpr int kClockSyncRetryIntervalMs = 2000;
} // namespace

SendingService::SendingService(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SendingService::sendNow);
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &SendingService::handleReplyFinished);
    connect(&m_notSentSender, &NotSentSender::sendSucceeded, this, &SendingService::sendSucceeded);
    connect(&m_notSentSender, &NotSentSender::sendFailed, this, &SendingService::sendFailed);
}

SendingService::~SendingService()
{
    m_notSentSender.stop();
}

QUrl SendingService::serverUrl() const
{
    QMutexLocker locker(&m_serverUrlMutex);
    return m_serverUrl;
}

void SendingService::setServerUrl(const QUrl &url)
{
    qCDebug(worktimeSendingService) << "setServerUrl called with" << url;
    {
        QMutexLocker locker(&m_serverUrlMutex);
        m_serverUrl = url;
    }
    m_notSentSender.setServerUrl(url);
    if (url.isValid())
        m_notSentSender.wake();
    emit serverUrlChanged();
}

void SendingService::start()
{
    qCInfo(worktimeSendingService) << "starting sending service";
    m_timer.setInterval(kNormalIntervalMs);
    m_timer.start();
    if (!m_notSentSender.isRunning())
        m_notSentSender.start();
    sendNow();
}

void SendingService::stop()
{
    qCInfo(worktimeSendingService) << "stopping sending service";
    m_timer.stop();
    m_notSentSender.stop();
}

bool SendingService::isActive() const
{
    return m_timer.isActive();
}

void SendingService::postSample(const QString &title, qint64 utcTimestamp, qint64 shootTime)
{
    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));
    request.setTransferTimeout(30000);

    const QByteArray body = buildShootRequestBody(title, utcTimestamp, shootTime);

    QNetworkReply *reply = m_networkManager.post(request, body);
    reply->setProperty("focusedWindowTitle", title);
    reply->setProperty("utcTimestamp", utcTimestamp);
    reply->setProperty("shootTime", shootTime);
    reply->setProperty("httpSize", qint64(body.size()));
}

void SendingService::sendNow()
{
    if (!m_serverUrl.isValid()) {
        qCWarning(worktimeSendingService) << "no server URL configured, skipping send";
        emit sendFailed(QStringLiteral("No server URL configured"));
        return;
    }

    if (!Utilities::isClockSyncEnabled()) {
        qCWarning(worktimeSendingService) << "automatic clock sync is disabled, skipping send";
        m_timer.setInterval(kClockSyncRetryIntervalMs);
        emit clockSyncDisabled();
        emit sendFailed(QStringLiteral("Automatic clock sync is disabled"));
        return;
    }
    m_timer.setInterval(kNormalIntervalMs);

    qCDebug(worktimeSendingService) << "posting activity sample to" << m_serverUrl;

    const QString title = Utilities::focusedWindowTitle();
    const qint64 utcTimestamp = QDateTime::currentSecsSinceEpoch();
    const qint64 shootTime = utcTimestamp + QDateTime::currentDateTime().offsetFromUtc();
    postSample(title, utcTimestamp, shootTime);
}

void SendingService::handleReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    const QString focusedWindowTitle = reply->property("focusedWindowTitle").toString();
    const qint64 utcTimestamp = reply->property("utcTimestamp").toLongLong();
    const qint64 shootTime = reply->property("shootTime").toLongLong();
    const qint64 httpSize = reply->property("httpSize").toLongLong();

    if (reply->error() != QNetworkReply::NoError) {
        const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        qCWarning(worktimeSendingService) << "submission failed (HTTP" << status << "):" << reply->errorString();

        // Force any future request on this manager to open a fresh connection,
        // in case this failure was caused by a network change (switching
        // WiFi/cable/VPN) leaving a pooled connection bound to a dead route.
        m_networkManager.clearConnectionCache();

        try {
            m_sqliteConnection.addEntryToNotSentTable(focusedWindowTitle, utcTimestamp, shootTime, httpSize);
        } catch (const std::exception &e) {
            qCCritical(worktimeSendingService) << "failed to record not-sent entry, sample is lost:" << e.what();
        }
        m_notSentSender.wake();

        if (status.toInt() == 401)
            emit authenticationFailed();

        emit sendFailed(reply->errorString());
        return;
    }

    qCInfo(worktimeSendingService) << "submission succeeded";

    try {
        m_sqliteConnection.addEntryToSentTable(focusedWindowTitle, utcTimestamp, shootTime, httpSize);
    } catch (const std::exception &e) {
        qCCritical(worktimeSendingService) << "failed to record sent entry:" << e.what();
    }

    emit sendSucceeded();
}
