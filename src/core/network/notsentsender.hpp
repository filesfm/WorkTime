#pragma once

#include "core/storage/sqliteconnection.hpp"

#include <QLoggingCategory>
#include <QMutex>
#include <QThread>
#include <QUrl>
#include <QWaitCondition>

#include <atomic>

Q_DECLARE_LOGGING_CATEGORY(worktimeNotSentSender)

/*!
 * \brief Worker thread with a single job: drain the `not_sent` table.
 *
 * Its run() loop takes the oldest `not_sent` entry (if any) and resends it,
 * pausing retryIntervalMs() between attempts so a long backlog doesn't burst
 * the server. When the table is empty it blocks on a condition variable
 * instead of polling, until wake() is called (e.g. because a new entry was
 * just queued) or stop() is requested.
 *
 * \note Thread-safety: setServerUrl(), setRetryIntervalMs(), wake() and
 * stop() may be called from any thread. Everything else (construction,
 * start(), destruction) must happen on the thread that owns this object, as
 * for any QThread.
 */
class NotSentSender : public QThread
{
    Q_OBJECT

public:
    explicit NotSentSender(QObject *parent = nullptr);
    /*! \brief Stops the worker loop (if running) and waits for it to exit. */
    ~NotSentSender() override;

    /*!
     * \brief Endpoint queued entries are resent to. Invalid/empty until set.
     * \note Thread safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setServerUrl(const QUrl &url);

    /*!
     * \brief Minimum delay between two consecutive resend attempts, in
     * milliseconds. Defaults to 5000.
     * \note Thread safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setRetryIntervalMs(int intervalMs);

    /*!
     * \brief Wakes the worker loop if it is currently blocked, so it
     * re-checks the `not_sent` table and serverUrl right away.
     * \note Thread safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void wake();

    /*!
     * \brief Requests the worker loop to exit and blocks until it has.
     * Safe to call even if the thread was never started.
     * \par Cyclomatic complexity: 1
     */
    void stop();

signals:
    /*! \brief Emitted after a queued entry was resent successfully. */
    void sendSucceeded();
    /*! \brief Emitted after resending a queued entry failed, with a human-readable reason. */
    void sendFailed(const QString &errorString);

protected:
    /*!
     * \brief The worker loop described in the class documentation.
     * \par Cyclomatic complexity: 6
     */
    void run() override;

private:
    /*!
     * \brief POSTs \a row to \a url and blocks (via a local event loop)
     * until the reply finishes.
     * \return An empty string on success, otherwise a human-readable error.
     * \par Cyclomatic complexity: 2
     */
    QString sendRowSynchronously(const QUrl &url, const SQLiteConnection::NotSentTableRow &row);

    QUrl serverUrl() const;
    int retryIntervalMs() const;

    /*! \brief Stores serverUrl */
    QUrl m_serverUrl;
    /*! \brief Guards m_serverUrl. */
    mutable QMutex m_serverUrlMutex;

    /*! \brief Stores retryIntervalMs */
    int m_retryIntervalMs = 5000;
    /*! \brief Guards m_retryIntervalMs. */
    mutable QMutex m_retryIntervalMutex;

    /*! \brief Guards m_wakeCondition and is held while checking m_stopRequested. */
    QMutex m_wakeMutex;
    /*! \brief Blocks run() while idle; see wake()/stop(). */
    QWaitCondition m_wakeCondition;
    /*! \brief Set by stop(); checked under m_wakeMutex to avoid missed wakeups. */
    bool m_stopRequested = false;
};
