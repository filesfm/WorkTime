#pragma once

#include "core/network/notsentsender.hpp"
#include "core/storage/sqliteconnection.hpp"

#include <QLoggingCategory>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(worktimeSendingService)

class QNetworkReply;

/*!
 * \brief Periodically submits one activity sample to the Worktime remote
 * endpoint, and drives a NotSentSender to drain any samples a past attempt
 * failed to deliver.
 */
class SendingService : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Wires the send timer, network manager and NotSentSender worker.
     * \par Cyclomatic complexity: 1
     */
    explicit SendingService(QObject *parent = nullptr);
    ~SendingService() override;

    /*!
     * \brief Endpoint requests are POSTed to. Invalid/empty until set.
     * \note Thread safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    QUrl serverUrl() const;
    /*!
     * \brief Sets serverUrl().
     * \note Thread safety: Thread-safe.
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
    void authenticationFailed();

private:
    /*!
     * \brief POSTs one activity sample built from \a title, \a utcTimestamp
     * and \a shootTime to serverUrl(), tagging the reply with them so
     * handleReplyFinished() can record the outcome.
     * \par Cyclomatic complexity: 1
     */
    void postSample(const QString &title, qint64 utcTimestamp, qint64 shootTime);

    /*!
     * \brief Captures the currently focused window and posts it via
     * postSample(), or emits sendFailed() if no valid URL is set.
     * \par Cyclomatic complexity: 2
     */
    void sendNow();
    /*!
     * \brief Deletes \a reply and emits sendSucceeded() or sendFailed()
     * depending on whether the request succeeded. On failure, also wakes
     * m_notSentSender so it picks up the newly queued entry without delay.
     * \par Cyclomatic complexity: 2
     */
    void handleReplyFinished(QNetworkReply *reply);

    /*! \brief Issues the POST requests built by buildShootRequestBody(). */
    QNetworkAccessManager m_networkManager;
    /*! \brief Drives the periodic sendNow() calls while active. */
    QTimer m_timer;
    /*! \brief Stores serverUrl */
    QUrl m_serverUrl;
    /*! \brief Guards m_serverUrl. */
    mutable QMutex m_serverUrlMutex;
    /*! \brief Records each send attempt's outcome in the `sent`/`not_sent` tables. */
    SQLiteConnection m_sqliteConnection;
    /*! \brief Drains `not_sent` in the background while active; see NotSentSender. */
    NotSentSender m_notSentSender;
};
