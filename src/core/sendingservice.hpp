#pragma once

#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(worktimeSendingService)

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
    /*! \brief Emmited after a server url changed. */
    void serverUrlChanged();
    /*! \brief Emitted after a send request completes successfully. */
    void sendSucceeded();
    /*! \brief Emitted after a send request fails, with a human-readable reason. */
    void sendFailed(const QString &errorString);

private:
    /*!
     * \brief Builds the request body for one activity sample.
     * \par Cyclomatic complexity: 1
     */
    QByteArray buildRequestBody() const;

    /*!
     * \brief Sends buildRequestBody() to serverUrl(), or emits sendFailed()
     * if no valid URL is set.
     * \par Cyclomatic complexity: 2
     */
    void sendNow();
    /*!
     * \brief Deletes \a reply and emits sendSucceeded() or sendFailed()
     * depending on whether the request succeeded.
     * \par Cyclomatic complexity: 2
     */
    void handleReplyFinished(QNetworkReply *reply);

    /*! \brief Issues the POST requests built by buildRequestBody(). */
    QNetworkAccessManager m_networkManager;
    /*! \brief Drives the periodic sendNow() calls while active. */
    QTimer m_timer;
    /*! \brief Stores serverUrl */
    QUrl m_serverUrl;
};
