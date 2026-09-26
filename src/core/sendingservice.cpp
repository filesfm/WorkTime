#include "sendingservice.hpp"

#include "core/settings.hpp"
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

Q_LOGGING_CATEGORY(worktimeSendingService, "worktime.sending.service")

namespace {

constexpr qsizetype kMaxMetadataLength = 255;

QByteArray encodeField(const QString &key, const QString &value)
{
    return QUrl::toPercentEncoding(key) + '=' + QUrl::toPercentEncoding(value);
}

} // namespace

SendingService::SendingService(QObject *parent)
    : QObject(parent)
{
    connect(&m_timer, &QTimer::timeout, this, &SendingService::sendNow);
    connect(&m_networkManager, &QNetworkAccessManager::finished, this, &SendingService::handleReplyFinished);
}

QUrl SendingService::serverUrl() const
{
    return m_serverUrl;
}

void SendingService::setServerUrl(const QUrl &url)
{
    qCDebug(worktimeSendingService) << "setServerUrl called with" << url;
    m_serverUrl = url;
    emit serverUrlChanged();
}

void SendingService::start()
{
    qCInfo(worktimeSendingService) << "starting sending service";
    m_timer.setInterval(60000);
    m_timer.start();
    sendNow();
}

void SendingService::stop()
{
    qCInfo(worktimeSendingService) << "stopping sending service";
    m_timer.stop();
}

bool SendingService::isActive() const
{
    return m_timer.isActive();
}

QByteArray SendingService::buildRequestBody(const QString &title, qint64 utcTimestamp, qint64 shootTime) const
{
    QByteArray body = encodeField(QStringLiteral("Shoot[user_name]"), Settings::instance()->username()) + '&'
                      + encodeField(QStringLiteral("Shoot[password]"), Settings::instance()->password()) + '&'
                      + encodeField(QStringLiteral("Shoot[project_name]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[app_name]"),
                                    Utilities::truncateUtf8Safe(title, kMaxMetadataLength))
                      + '&' + encodeField(QStringLiteral("Shoot[document_name]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[document_path]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[shoot_time]"), QString::number(shootTime)) + '&'
                      + encodeField(QStringLiteral("Shoot[utc_timestamp]"), QString::number(utcTimestamp)) + '&'
                      + encodeField(QStringLiteral("Shoot[image_resized]"), QStringLiteral("1"));

    return body;
}

void SendingService::sendNow()
{
    if (!m_serverUrl.isValid()) {
        qCWarning(worktimeSendingService) << "no server URL configured, skipping send";
        emit sendFailed(QStringLiteral("No server URL configured"));
        return;
    }

    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));

    qCDebug(worktimeSendingService) << "posting activity sample to" << m_serverUrl;

    const QString title = Utilities::focusedWindowTitle();
    const qint64 utcTimestamp = QDateTime::currentSecsSinceEpoch();
    const qint64 shootTime = utcTimestamp + QDateTime::currentDateTime().offsetFromUtc();
    const QByteArray body = buildRequestBody(title, utcTimestamp, shootTime);

    QNetworkReply *reply = m_networkManager.post(request, body);
    reply->setProperty("focusedWindowTitle", title);
    reply->setProperty("utcTimestamp", utcTimestamp);
    reply->setProperty("shootTime", shootTime);
    reply->setProperty("httpSize", qint64(body.size()));
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

        m_sqliteConnection.addEntryToNotSentTable(focusedWindowTitle, utcTimestamp, shootTime, httpSize);

        emit sendFailed(reply->errorString());
        return;
    }

    qCInfo(worktimeSendingService) << "submission succeeded";

    m_sqliteConnection.addEntryToSentTable(focusedWindowTitle, utcTimestamp, shootTime, httpSize);

    emit sendSucceeded();
}
