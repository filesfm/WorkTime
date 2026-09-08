#include "sendingservice.hpp"

#include "core/settings.hpp"
#include "core/utilities.hpp"

#include <QDateTime>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

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

    connect(Settings::instance(), &Settings::postIntervalChanged, this, [this] {
        if (isActive())
            m_timer.setInterval(Settings::instance()->postInterval() * 1000);
    });
}

QUrl SendingService::serverUrl() const
{
    return m_serverUrl;
}

void SendingService::setServerUrl(const QUrl &url)
{
    m_serverUrl = url;
}

void SendingService::start()
{
    m_timer.setInterval(Settings::instance()->postInterval() * 1000);
    m_timer.start();
    sendNow();
}

void SendingService::stop()
{
    m_timer.stop();
}

bool SendingService::isActive() const
{
    return m_timer.isActive();
}

QByteArray SendingService::buildRequestBody() const
{
    const qint64 utcTimestamp = QDateTime::currentSecsSinceEpoch();
    const qint64 shootTime = utcTimestamp + QDateTime::currentDateTime().offsetFromUtc();

    const QByteArray body
        = encodeField(QStringLiteral("Shoot[user_name]"), Settings::instance()->username()) + '&'
          + encodeField(QStringLiteral("Shoot[password]"), Settings::instance()->password()) + '&'
          + encodeField(QStringLiteral("Shoot[project_name]"), QString()) + '&'
          + encodeField(QStringLiteral("Shoot[app_name]"),
                        Utilities::truncateUtf8Safe(Utilities::focusedApplicationName(), kMaxMetadataLength))
          + '&'
          + encodeField(QStringLiteral("Shoot[document_name]"),
                        Utilities::truncateUtf8Safe(Utilities::activeWindowTitle(), kMaxMetadataLength))
          + '&'
          + encodeField(QStringLiteral("Shoot[document_path]"),
                        Utilities::truncateUtf8Safe(Utilities::activeWindowExecutablePath(), kMaxMetadataLength))
          + '&' + encodeField(QStringLiteral("Shoot[shoot_time]"), QString::number(shootTime)) + '&'
          + encodeField(QStringLiteral("Shoot[utc_timestamp]"), QString::number(utcTimestamp)) + '&'
          + encodeField(QStringLiteral("Shoot[image_resized]"), QStringLiteral("1"));

    return body;
}

void SendingService::sendNow()
{
    if (!m_serverUrl.isValid()) {
        emit sendFailed(QStringLiteral("No server URL configured"));
        return;
    }

    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));

    m_networkManager.post(request, buildRequestBody());
}

void SendingService::handleReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QVariant status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
        qWarning() << "SendingService: submission failed (HTTP" << status << "):" << reply->errorString();
        emit sendFailed(reply->errorString());
        return;
    }

    emit sendSucceeded();
}
