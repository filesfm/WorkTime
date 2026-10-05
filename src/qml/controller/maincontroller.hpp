#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QTimer>
#include <QUrl>

#include <atomic>

#include "core/activity/activitymonitor.hpp"
#include "core/network/sendingservice.hpp"
#include "core/storage/sqliteconnection.hpp"

/*!
 * \brief QML-facing controller behind Main.qml.
 *
 * Instantiated directly from QML (not a singleton). Owns whether time
 * tracking is currently active, and acts as the sole QML-facing facade over
 * Settings — Settings itself stays plain C++ and is never exposed to QML
 * directly; MainController's setters forward to it, and its property
 * change signals are relayed from Settings' own.
 */
class MainController : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
#if !defined(BUILD_WITHOUT_AUTOSTART)
    Q_PROPERTY(bool autoStartup READ autoStartup WRITE setAutoStartup NOTIFY autoStartupChanged)
#endif
    Q_PROPERTY(bool startButtonPushed READ startButtonPushed WRITE setStartButtonPushed NOTIFY startButtonChanged)
    Q_PROPERTY(qint64 sentCountLast24Hours READ sentCountLast24Hours NOTIFY sentStatsChanged)
    Q_PROPERTY(qint64 sentBytesLast24Hours READ sentBytesLast24Hours NOTIFY sentStatsChanged)
    Q_PROPERTY(UserStatus userStatus READ userStatus NOTIFY userStatusChanged)
    Q_PROPERTY(QUrl statusIconSource READ statusIconSource NOTIFY userStatusChanged)
    Q_PROPERTY(bool credentialsRejected READ credentialsRejected NOTIFY credentialsRejectedChanged)

public:
    //! \brief User's current activity/connectivity state; see setUserStatus().
    enum class UserStatus : std::int8_t { ACTIVE, INACTIVE, ERROR };
    Q_ENUM(UserStatus)

    /*!
     * \brief Relays every Settings property-change signal to this controller's own.
     * Also extracts the status tray icons (resource/icon{,-green,-red}.png) to disk
     * once, since Qt.labs.platform's SystemTrayIcon can't load them straight out of
     * the Qt resource system (see Utilities::extractResourceToDisk()).
     * \par Cyclomatic complexity: 1
     */
    explicit MainController(QObject *parent = nullptr);

    /*!
     * \brief Starts tracking if stopped, stops it if running.
     * Emits runningChanged() if the state changes.
     * \par Cyclomatic complexity: 1
     */
    Q_INVOKABLE void toggleTracking();

    /*!
     * \brief Forwards to Settings::username().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    QString username() const;
    /*!
     * \brief Forwards to Settings::setUsername().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setUsername(const QString &username) const;

    /*!
     * \brief Forwards to Settings::password().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    QString password() const;
    /*!
     * \brief Forwards to Settings::setPassword().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setPassword(const QString &password) const;

#if !defined(BUILD_WITHOUT_AUTOSTART)
    /*!
     * \brief Forwards to Settings::autoStartup().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    bool autoStartup() const;
    /*!
     * \brief Forwards to Settings::setAutoStartup().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setAutoStartup(bool enabled) const;
#endif

    /*!
     * \brief Forwards to Settings::startButtonPushed().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    bool startButtonPushed() const;
    /*!
     * \brief Forwards to Settings::setStartButtonPushed().
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    void setStartButtonPushed(bool pushed) const;

    /*!
     * \brief Number of `sent` table entries from the last 24 hours, cached
     * and refreshed by refreshSentStats().
     * \par Cyclomatic complexity: 1
     */
    qint64 sentCountLast24Hours() const;
    /*!
     * \brief Sum of `http_size` for `sent` table entries from the last 24
     * hours, cached and refreshed by refreshSentStats().
     * \par Cyclomatic complexity: 1
     */
    qint64 sentBytesLast24Hours() const;

    /*!
     * \brief ACTIVE while startButtonPushed() is true, INACTIVE while it's
     * false, or ERROR while the last send attempt failed (see
     * SendingService::sendFailed()). Independent of the activity-based
     * m_userStatus setUserStatus() maintains, which only governs pausing
     * SendingService while the user is idle.
     * \par Cyclomatic complexity: 1
     */
    UserStatus userStatus() const;
    /*!
     * \brief The tray icon matching userStatus(): green for ACTIVE, red for
     * ERROR, the default icon for INACTIVE.
     * \par Cyclomatic complexity: 1
     */
    QUrl statusIconSource() const;

    bool credentialsRejected() const;

signals:
    void usernameChanged();
    void passwordChanged();
#if !defined(BUILD_WITHOUT_AUTOSTART)
    void autoStartupChanged();
#endif
    void startButtonChanged() const;
    void userStatusChanged();
    /*! \brief Emitted after sentCountLast24Hours() or sentBytesLast24Hours() changes. */
    void sentStatsChanged();
    void credentialsRejectedChanged();

private:
    /*!
     * \brief Wires up the inactivity timer and the global activity monitor, then
     * starts both: activityMonitor's activityDetected() marks the user ACTIVE and
     * restarts the 1-minute timer; the timer firing (no activity for a minute)
     * marks the user INACTIVE.
     * \par Cyclomatic complexity: 1
     */
    void activityChecker();

    /*!
     * \brief Sets m_userStatus, starting/stopping the sending service to match
     * whenever tracking is running (see startButtonPushed()).
     * \par Cyclomatic complexity: 3
     */
    void setUserStatus(UserStatus status);

    /*!
     * \brief Re-reads the last-24-hours sent count/bytes from m_sqliteConnection
     * and emits sentStatsChanged() if either changed.
     * \par Cyclomatic complexity: 2
     */
    void refreshSentStats();

    void setCredentialsRejected(bool rejected);

private slots:
    void showStartButtonNotPushedNotification();
    void toggleStartButtonNotPushedTimer();

private:
    void setRunning(bool running);

    std::atomic<UserStatus> m_userStatus;
    SendingService m_sendingService;
    ActivityMonitor m_activityMonitor;
    QTimer m_inactivityTimer;
    QTimer m_startButtonNotPushedTimer;

    /*! \brief Backs sentCountLast24Hours()/sentBytesLast24Hours(); see refreshSentStats(). */
    SQLiteConnection m_sqliteConnection;
    qint64 m_sentCountLast24Hours = 0;
    qint64 m_sentBytesLast24Hours = 0;
    /*! \brief Periodically calls refreshSentStats() so the 24h window keeps moving even without new sends. */
    QTimer m_sentStatsRefreshTimer;

    /*! \brief Backs the ERROR branch of userStatus(); set/cleared by SendingService::sendFailed()/sendSucceeded(). */
    bool m_hasSendError = false;

    bool m_credentialsRejected = false;

    /*! \brief statusIconSource() for UserStatus::ACTIVE; extracted from resource/icon-green.png. */
    QUrl m_activeIconUrl;
    /*! \brief statusIconSource() for UserStatus::INACTIVE; extracted from resource/icon.png. */
    QUrl m_inactiveIconUrl;
    /*! \brief statusIconSource() for UserStatus::ERROR; extracted from resource/icon-red.png. */
    QUrl m_errorIconUrl;
};
