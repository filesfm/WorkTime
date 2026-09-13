#pragma once

#include <QObject>
#include <QQmlEngine>

#include "core/sendingservice.hpp"

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
    Q_PROPERTY(bool autoStartup READ autoStartup WRITE setAutoStartup NOTIFY autoStartupChanged)
    Q_PROPERTY(bool startButtonPushed READ startButtonPushed WRITE setStartButtonPushed NOTIFY startButtonChanged)

public:
    /*!
     * \brief Relays every Settings property-change signal to this controller's own.
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
     * \par Cyclomatic complexity: 1
     */
    QString username() const;
    /*!
     * \brief Forwards to Settings::setUsername().
     * \par Cyclomatic complexity: 1
     */
    void setUsername(const QString &username) const;

    /*!
     * \brief Forwards to Settings::password().
     * \par Cyclomatic complexity: 1
     */
    QString password() const;
    /*!
     * \brief Forwards to Settings::setPassword().
     * \par Cyclomatic complexity: 1
     */
    void setPassword(const QString &password) const;

    /*!
     * \brief Forwards to Settings::autoStartup().
     * \par Cyclomatic complexity: 1
     */
    bool autoStartup() const;
    /*!
     * \brief Forwards to Settings::setAutoStartup().
     * \par Cyclomatic complexity: 1
     */
    void setAutoStartup(bool enabled) const;

    /*!
     * \brief Forwards to Settings::startButtonPushed().
     * \par Cyclomatic complexity: 1
     */
    bool startButtonPushed() const;
    /*!
     * \brief Forwards to Settings::setStartButtonPushed().
     * \par Cyclomatic complexity: 1
     */
    void setStartButtonPushed(bool pushed) const;

signals:
    void usernameChanged();
    void passwordChanged();
    void autoStartupChanged();
    void startButtonChanged() const;

private:
    void setRunning(bool running);

    SendingService m_sendingService;
};
