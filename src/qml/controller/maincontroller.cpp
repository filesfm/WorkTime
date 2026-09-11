#include "maincontroller.hpp"

#include "core/settings.hpp"

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    m_sendingService.setServerUrl(QUrl(QStringLiteral("https://worktime.lv/remote")));

    connect(Settings::instance(), &Settings::usernameChanged, this, &MainController::usernameChanged);
    connect(Settings::instance(), &Settings::passwordChanged, this, &MainController::passwordChanged);
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::autoStartupChanged);
}

bool MainController::running() const
{
    return m_running;
}

void MainController::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    if (m_running)
        m_sendingService.start();
    else
        m_sendingService.stop();
    emit runningChanged();
}

void MainController::toggleTracking()
{
    setRunning(!m_running);
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
