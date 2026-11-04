#include "maincontroller.hpp"

#include "core/settings/settings.hpp"
#include "core/utilities.hpp"

#include <QDateTime>

#include <exception>

Q_LOGGING_CATEGORY(worktimeMainController, "worktime.main.controller")

namespace {
constexpr int kInactivityTimeoutMs = 60000;
constexpr int kStartButtonNotPushedTimeoutMs = 1800000;
constexpr int kSentStatsRefreshIntervalMs = 60000;
// SendingService emits clockSyncDisabled() on every retry attempt while OS
// clock sync stays off (as often as every couple of seconds, see
// SendingService's own retry-interval constant) - throttle the actual
// desktop notification to at most once per minute rather than spamming one
// per retry.
constexpr qint64 kClockSyncNotificationIntervalMs = 60000;
} // namespace

void MainController::activityChecker()
{
    m_inactivityTimer.setSingleShot(true);
    m_inactivityTimer.setInterval(kInactivityTimeoutMs);
    m_startButtonNotPushedTimer.setSingleShot(false);
    m_startButtonNotPushedTimer.setInterval(kStartButtonNotPushedTimeoutMs);

    connect(&m_inactivityTimer, &QTimer::timeout, this, [this]() { setUserStatus(UserStatus::INACTIVE); });
    connect(&m_startButtonNotPushedTimer, &QTimer::timeout, this, &MainController::showStartButtonNotPushedNotification);
    connect(this, &MainController::startButtonChanged, this, &MainController::toggleStartButtonNotPushedTimer);

    connect(&m_activityMonitor, &ActivityMonitor::activityDetected, this, [this]() {
        setUserStatus(UserStatus::ACTIVE);
        m_inactivityTimer.start();
    });

    m_activityMonitor.start();
    m_inactivityTimer.start();

    if (startButtonPushed()) {
        m_startButtonNotPushedTimer.stop();
    } else {
        m_startButtonNotPushedTimer.start();
    }
}

void MainController::notifyClockSyncDisabled()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastClockSyncNotificationMs < kClockSyncNotificationIntervalMs)
        return;

    m_lastClockSyncNotificationMs = now;
    Utilities::showNotification("WorkTime",
                                "Automatic time sync appears to be turned off. Please turn it back on in your "
                                "system clock settings so WorkTime keeps tracking accurate time.");
}

void MainController::setUserStatus(UserStatus status)
{
    if (m_userStatus.exchange(status) == status)
        return;

    emit userStatusChanged();

    if (!startButtonPushed())
        return;

    if (status == UserStatus::ACTIVE)
        m_sendingService.start();
    else
        m_sendingService.stop();
}

MainController::MainController(QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_LINUX)
    // Qt.labs.platform's Linux (DBus StatusNotifierItem) tray backend can't
    // load icons straight out of the Qt resource system, so they're copied
    // to real files on disk once at startup. Other platforms load the qrc
    // resource directly below - they don't have this limitation, and the
    // disk copy would just be an extra way for the tray icon to end up
    // broken (e.g. if TempLocation isn't writable on some machine).
    m_activeIconUrl = Utilities::extractResourceToDisk(QStringLiteral(":/qt/qml/Worktime/icon-green.png"),
                                                       QStringLiteral("worktime-tray-icon-active.png"));
    m_inactiveIconUrl = Utilities::extractResourceToDisk(QStringLiteral(":/qt/qml/Worktime/icon.png"),
                                                         QStringLiteral("worktime-tray-icon-inactive.png"));
    m_errorIconUrl = Utilities::extractResourceToDisk(QStringLiteral(":/qt/qml/Worktime/icon-red.png"),
                                                      QStringLiteral("worktime-tray-icon-error.png"));
#else
    m_activeIconUrl = QUrl(QStringLiteral("qrc:/qt/qml/Worktime/icon-green.png"));
    m_inactiveIconUrl = QUrl(QStringLiteral("qrc:/qt/qml/Worktime/icon.png"));
    m_errorIconUrl = QUrl(QStringLiteral("qrc:/qt/qml/Worktime/icon-red.png"));
#endif

    m_sendingService.setServerUrl(QUrl(QStringLiteral("https://worktime.lv/remote")));

    connect(Settings::instance(), &Settings::usernameChanged, this, &MainController::usernameChanged);
    connect(Settings::instance(), &Settings::passwordChanged, this, &MainController::passwordChanged);
#if !defined(BUILD_WITHOUT_AUTOSTART)
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::autoStartupChanged);
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::startButtonChanged);
#endif

    if (startButtonPushed() && m_userStatus == UserStatus::ACTIVE) {
        m_sendingService.start();
    } else {
        m_sendingService.stop();
    }

    activityChecker();
    connect(&m_sendingService, &SendingService::clockSyncDisabled, this, &MainController::notifyClockSyncDisabled);

    connect(&m_sendingService, &SendingService::sendSucceeded, this, &MainController::refreshSentStats);
    connect(&m_sendingService, &SendingService::sendFailed, this, &MainController::refreshSentStats);
    connect(&m_sentStatsRefreshTimer, &QTimer::timeout, this, &MainController::refreshSentStats);
    m_sentStatsRefreshTimer.setInterval(kSentStatsRefreshIntervalMs);
    m_sentStatsRefreshTimer.start();
    refreshSentStats();

    connect(this, &MainController::startButtonChanged, this, &MainController::userStatusChanged);
    connect(&m_sendingService, &SendingService::sendFailed, this, [this](const QString &) {
        if (m_hasSendError)
            return;
        m_hasSendError = true;
        emit userStatusChanged();
    });
    connect(&m_sendingService, &SendingService::sendSucceeded, this, [this]() {
        if (!m_hasSendError)
            return;
        m_hasSendError = false;
        emit userStatusChanged();
    });

    connect(&m_sendingService, &SendingService::authenticationFailed, this, [this]() { setCredentialsRejected(true); });
    connect(&m_sendingService, &SendingService::sendSucceeded, this, [this]() { setCredentialsRejected(false); });
    connect(this, &MainController::usernameChanged, this, [this]() { setCredentialsRejected(false); });
    connect(this, &MainController::passwordChanged, this, [this]() { setCredentialsRejected(false); });
}

bool MainController::credentialsRejected() const
{
    return m_credentialsRejected;
}

void MainController::setCredentialsRejected(bool rejected)
{
    if (m_credentialsRejected == rejected)
        return;
    m_credentialsRejected = rejected;
    emit credentialsRejectedChanged();
}

qint64 MainController::sentCountLast24Hours() const
{
    return m_sentCountLast24Hours;
}

qint64 MainController::sentBytesLast24Hours() const
{
    return m_sentBytesLast24Hours;
}

MainController::UserStatus MainController::userStatus() const
{
    if (m_hasSendError)
        return UserStatus::ERROR;
    if (!startButtonPushed())
        return UserStatus::INACTIVE;
    return m_userStatus.load();
}

QUrl MainController::statusIconSource() const
{
    switch (userStatus()) {
    case UserStatus::ACTIVE:
        return m_activeIconUrl;
    case UserStatus::ERROR:
        return m_errorIconUrl;
    case UserStatus::INACTIVE:
        return m_inactiveIconUrl;
    }
    return m_inactiveIconUrl;
}

void MainController::refreshSentStats()
{
    qint64 count;
    qint64 bytes;
    try {
        count = m_sqliteConnection.countEntriesInLast24HoursFromSentTable();
        bytes = m_sqliteConnection.sumHttpSizeInLast24HoursFromSentTable();
    } catch (const std::exception &e) {
        qCCritical(worktimeMainController) << "failed to refresh sent stats:" << e.what();
        return;
    }

    if (count == m_sentCountLast24Hours && bytes == m_sentBytesLast24Hours)
        return;

    m_sentCountLast24Hours = count;
    m_sentBytesLast24Hours = bytes;
    emit sentStatsChanged();
}

void MainController::setRunning(bool running)
{
    if (startButtonPushed() == running)
        return;
    setStartButtonPushed(running);
    if (running && m_userStatus == UserStatus::ACTIVE)
        m_sendingService.start();
    else
        m_sendingService.stop();
    emit startButtonChanged();
}

void MainController::toggleTracking()
{
    setRunning(!startButtonPushed());
}

QString MainController::username() const
{
    return Settings::instance()->username();
}

void MainController::setUsername(const QString &username) const
{
    Settings::instance()->setUsername(username);
}

QString MainController::password() const
{
    return Settings::instance()->password();
}

void MainController::setPassword(const QString &password) const
{
    Settings::instance()->setPassword(password);
}

#if !defined(BUILD_WITHOUT_AUTOSTART)

bool MainController::autoStartup() const
{
    return Settings::instance()->autoStartup();
}

void MainController::setAutoStartup(bool enabled) const
{
    Settings::instance()->setAutoStartup(enabled);
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART)

bool MainController::startButtonPushed() const
{
    return Settings::instance()->startButtonPushed();
}

void MainController::setStartButtonPushed(bool pushed) const
{
    Settings::instance()->setStartButtonPushed(pushed);
    emit startButtonChanged();
}

void MainController::showStartButtonNotPushedNotification()
{
    Utilities::showNotification("WorkTime", "WorkTime is launched but the Start button is not pushed!");
}

void MainController::toggleStartButtonNotPushedTimer()
{
    if (startButtonPushed()) {
        m_startButtonNotPushedTimer.stop();
    } else {
        m_startButtonNotPushedTimer.start();
    }
}
