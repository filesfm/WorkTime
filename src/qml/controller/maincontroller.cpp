#include "maincontroller.hpp"

#include "core/settings.hpp"

MainController::MainController(QObject *parent)
    : QObject(parent)
{
    connect(Settings::instance(), &Settings::usernameChanged, this, &MainController::usernameChanged);
    connect(Settings::instance(), &Settings::passwordChanged, this, &MainController::passwordChanged);
    connect(Settings::instance(), &Settings::autoStartupChanged, this, &MainController::autoStartupChanged);
    connect(Settings::instance(), &Settings::postIntervalChanged, this, &MainController::postIntervalChanged);
    connect(Settings::instance(), &Settings::enableShotsChanged, this, &MainController::enableShotsChanged);
    connect(Settings::instance(), &Settings::shotScaleChanged, this, &MainController::shotScaleChanged);
    connect(Settings::instance(), &Settings::shotCompressionChanged, this, &MainController::shotCompressionChanged);
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

int MainController::postInterval() const
{
    return Settings::instance()->postInterval();
}

void MainController::setPostInterval(int seconds) const
{
    Settings::instance()->setPostInterval(seconds);
}

bool MainController::enableShots() const
{
    return Settings::instance()->enableShots();
}

void MainController::setEnableShots(bool enabled) const
{
    Settings::instance()->setEnableShots(enabled);
}

qreal MainController::shotScale() const
{
    return Settings::instance()->shotScale();
}

void MainController::setShotScale(qreal scale) const
{
    Settings::instance()->setShotScale(scale);
}

int MainController::shotCompression() const
{
    return Settings::instance()->shotCompression();
}

void MainController::setShotCompression(int compression) const
{
    Settings::instance()->setShotCompression(compression);
}
