#include "sendingservice.hpp"

#include "core/settings.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>

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

QJsonObject SendingService::buildPayload() const
{
    // TODO: replace with the real payload shape once the server API is defined.
    return QJsonObject{
        {"username", Settings::instance()->username()},
        {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
}

void SendingService::sendNow()
{
    if (!m_serverUrl.isValid()) {
        emit sendFailed(QStringLiteral("No server URL configured"));
        return;
    }

    QNetworkRequest request(m_serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    const QJsonDocument document(buildPayload());
    m_networkManager.post(request, document.toJson(QJsonDocument::Compact));
}

void SendingService::handleReplyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        emit sendFailed(reply->errorString());
        return;
    }

    emit sendSucceeded();
}
