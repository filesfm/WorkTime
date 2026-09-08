#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

class QNetworkReply;

/*!
 * \brief Periodically submits one activity sample to the Worktime remote
 * endpoint.
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
     * \brief Endpoint requests are POSTed to. Invalid/empty until set.
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
     * \brief Builds the `application/x-www-form-urlencoded` request body for
     * one activity sample, per the Worktime remote submission protocol:
     * credentials and activity metadata as `Shoot[...]` fields, with
     * `shoot_time`/`utc_timestamp` computed for the current instant.
     *
     * Credentials (`Shoot[user_name]`/`Shoot[password]`) are sent verbatim;
     * metadata fields are truncated to 255 Unicode code points.
     * \par Cyclomatic complexity: 1
     */
    QByteArray buildRequestBody() const;

    void sendNow();
    void handleReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager m_networkManager;
    QTimer m_timer;
    QUrl m_serverUrl;
};
