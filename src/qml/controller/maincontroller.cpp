#include "maincontroller.hpp"

#include "core/settings.hpp"
#include "core/utilities.hpp"

namespace {
constexpr int kInactivityTimeoutMs = 60000;
constexpr int kStartButtonNotPushedTimeoutMs = 1800000;
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

void MainController::setUserStatus(UserStatus status)
{
    if (userStatus.exchange(status) == status)
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
    m_sendingService.setServerUrl(QUrl(QStringLiteral("https://worktime.lv/remote")));

    connect(Settings::instance(), &Settings::usernameChanged, this, &MainController::usernameChanged);
    connect(Settings::instance(), &Settings::passwordChanged, this, &MainController::passwordChanged);
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::autoStartupChanged);
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::startButtonChanged);

    if (startButtonPushed() && userStatus == UserStatus::ACTIVE) {
        m_sendingService.start();
    } else {
        m_sendingService.stop();
    }

    activityChecker();
}

void MainController::setRunning(bool running)
{
    if (startButtonPushed() == running)
        return;
    setStartButtonPushed(running);
    if (running && userStatus == UserStatus::ACTIVE)
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

bool MainController::autoStartup() const
{
    return Settings::instance()->autoStartup();
}

void MainController::setAutoStartup(bool enabled) const
{
    Settings::instance()->setAutoStartup(enabled);
}

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
