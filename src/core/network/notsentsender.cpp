#include "notsentsender.hpp"

#include "core/network/shootrequestbuilder.hpp"

#include <QEventLoop>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

Q_LOGGING_CATEGORY(worktimeNotSentSender, "worktime.notsent.sender")

namespace {
constexpr int kReplyTimeoutMs = 30000;
} // namespace

NotSentSender::NotSentSender(QObject *parent)
    : QThread(parent)
{}

NotSentSender::~NotSentSender()
{
    stop();
}

void NotSentSender::setServerUrl(const QUrl &url)
{
    QMutexLocker locker(&m_serverUrlMutex);
    m_serverUrl = url;
}

QUrl NotSentSender::serverUrl() const
{
    QMutexLocker locker(&m_serverUrlMutex);
    return m_serverUrl;
}

void NotSentSender::setRetryIntervalMs(int intervalMs)
{
    QMutexLocker locker(&m_retryIntervalMutex);
    m_retryIntervalMs = intervalMs;
}

int NotSentSender::retryIntervalMs() const
{
    QMutexLocker locker(&m_retryIntervalMutex);
    return m_retryIntervalMs;
}

void NotSentSender::wake()
{
    QMutexLocker locker(&m_wakeMutex);
    m_wakeCondition.wakeAll();
}

void NotSentSender::stop()
{
    {
        QMutexLocker locker(&m_wakeMutex);
        m_stopRequested = true;
        m_wakeCondition.wakeAll();
    }
    wait();
}

QString NotSentSender::sendRowSynchronously(const QUrl &url, const SQLiteConnection::NotSentTableRow &row)
{
    QNetworkAccessManager networkManager;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/x-www-form-urlencoded; charset=UTF-8"));
    request.setTransferTimeout(kReplyTimeoutMs);

    const QByteArray body = buildShootRequestBody(row.focusedWindowTitle, row.utcTimestamp, row.shootTime);

    QEventLoop loop;
    QNetworkReply *reply = networkManager.post(request, body);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QString errorString = reply->error() == QNetworkReply::NoError ? QString() : reply->errorString();
    reply->deleteLater();
    return errorString;
}

void NotSentSender::run()
{
    // SQLiteConnection disables operator new/placement-new to force
    // stack-only use, so it's constructed here as a plain local - inside the
    // try so a failure to open the database (e.g. disk full) logs and ends
    // this thread instead of propagating out of run() and terminating the
    // whole application.
    try {
        SQLiteConnection connection;

        while (true) {
            const QUrl url = serverUrl();
            std::optional<SQLiteConnection::NotSentTableRow> row;

            if (url.isValid()) {
                try {
                    row = connection.takeOldestEntryFromNotSentTable();
                } catch (const std::exception &e) {
                    qCWarning(worktimeNotSentSender) << "failed to read not_sent table:" << e.what();
                }
            }

            if (!row) {
                QMutexLocker locker(&m_wakeMutex);
                if (m_stopRequested)
                    return;
                m_wakeCondition.wait(&m_wakeMutex);
                if (m_stopRequested)
                    return;
                continue;
            }

            qCDebug(worktimeNotSentSender) << "retrying queued not_sent entry";
            const QString error = sendRowSynchronously(url, *row);
            const bool success = error.isEmpty();

            try {
                if (success) {
                    connection.addEntryToSentTable(row->focusedWindowTitle,
                                                   row->utcTimestamp,
                                                   row->shootTime,
                                                   row->httpSize);
                } else {
                    connection.addEntryToNotSentTable(row->focusedWindowTitle,
                                                      row->utcTimestamp,
                                                      row->shootTime,
                                                      row->httpSize);
                }
            } catch (const std::exception &e) {
                qCWarning(worktimeNotSentSender) << "failed to record resend outcome:" << e.what();
            }

            if (success) {
                qCInfo(worktimeNotSentSender) << "resent queued entry successfully";
                emit sendSucceeded();
            } else {
                qCWarning(worktimeNotSentSender) << "resend failed:" << error;
                emit sendFailed(error);
            }

            QMutexLocker locker(&m_wakeMutex);
            if (m_stopRequested)
                return;
            m_wakeCondition.wait(&m_wakeMutex, retryIntervalMs());
            if (m_stopRequested)
                return;
        }
    } catch (const std::exception &e) {
        qCCritical(worktimeNotSentSender)
            << "failed to open database, not_sent entries will not be retried:" << e.what();
    }
}
