#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

/*!
 * \brief Periodically sends tracked-time data to a remote server.
 *
 * The concrete server API is not yet defined, so this class is deliberately
 * generic: it owns the send schedule (driven by Settings::postInterval())
 * and the HTTP transport, while the request URL and payload shape - the
 * parts that depend on the actual API - are isolated behind serverUrl()
 * and buildPayload() so they can be filled in later without touching the
 * scheduling/transport logic around them.
 */
class SendingService : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Wires the send timer and network manager, and rebinds the timer interval
     * whenever Settings::postInterval() changes.
     * \par Cyclomatic complexity: 1
     */
    explicit SendingService(QObject *parent = nullptr);

    /*!
     * \brief Endpoint requests are POSTed to. Invalid/empty until the API is known.
     * \par Cyclomatic complexity: 1
     */
    QUrl serverUrl() const;
    /*!
     * \brief Sets serverUrl().
     * \par Cyclomatic complexity: 1
     */
    void setServerUrl(const QUrl &url);

    /*!
     * \brief Starts the periodic send timer and sends immediately once.
     * \par Cyclomatic complexity: 1
     */
    void start();
    /*!
     * \brief Stops the periodic send timer.
     * \par Cyclomatic complexity: 1
     */
    void stop();
    /*!
     * \brief Whether the periodic send timer is currently active.
     * \par Cyclomatic complexity: 1
     */
    bool isActive() const;

signals:
    /*! \brief Emitted after a send request completes successfully. */
    void sendSucceeded();
    /*! \brief Emitted after a send request fails, with a human-readable reason. */
    void sendFailed(const QString &errorString);

private:
    /*!
     * \brief Builds the JSON payload for one send.
     *
     * Placeholder shape (username + UTC timestamp) until the actual API
     * contract is known - change freely, nothing else depends on its
     * fields.
     * \par Cyclomatic complexity: 1
     */
    QJsonObject buildPayload() const;

    void sendNow();
    void handleReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager m_networkManager;
    QTimer m_timer;
    QUrl m_serverUrl;
};
