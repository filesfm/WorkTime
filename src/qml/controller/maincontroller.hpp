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

    /*! \brief Whether tracking is currently active. */
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)

    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(bool autoStartup READ autoStartup WRITE setAutoStartup NOTIFY autoStartupChanged)
    Q_PROPERTY(int postInterval READ postInterval WRITE setPostInterval NOTIFY postIntervalChanged)
    Q_PROPERTY(bool enableShots READ enableShots WRITE setEnableShots NOTIFY enableShotsChanged)
    Q_PROPERTY(qreal shotScale READ shotScale WRITE setShotScale NOTIFY shotScaleChanged)
    Q_PROPERTY(int shotCompression READ shotCompression WRITE setShotCompression NOTIFY shotCompressionChanged)

public:
    /*!
     * \brief Relays every Settings property-change signal to this controller's own.
     * \par Cyclomatic complexity: 1
     */
    explicit MainController(QObject *parent = nullptr);

    bool running() const;

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
    void setUsername(const QString &username);

    /*!
     * \brief Forwards to Settings::password().
     * \par Cyclomatic complexity: 1
     */
    QString password() const;
    /*!
     * \brief Forwards to Settings::setPassword().
     * \par Cyclomatic complexity: 1
     */
    void setPassword(const QString &password);

    /*!
     * \brief Forwards to Settings::autoStartup().
     * \par Cyclomatic complexity: 1
     */
    bool autoStartup() const;
    /*!
     * \brief Forwards to Settings::setAutoStartup().
     * \par Cyclomatic complexity: 1
     */
    void setAutoStartup(bool enabled);

    /*!
     * \brief Forwards to Settings::postInterval().
     * \par Cyclomatic complexity: 1
     */
    int postInterval() const;
    /*!
     * \brief Forwards to Settings::setPostInterval().
     * \par Cyclomatic complexity: 1
     */
    void setPostInterval(int seconds);

    /*!
     * \brief Forwards to Settings::enableShots().
     * \par Cyclomatic complexity: 1
     */
    bool enableShots() const;
    /*!
     * \brief Forwards to Settings::setEnableShots().
     * \par Cyclomatic complexity: 1
     */
    void setEnableShots(bool enabled);

    /*!
     * \brief Forwards to Settings::shotScale().
     * \par Cyclomatic complexity: 1
     */
    qreal shotScale() const;
    /*!
     * \brief Forwards to Settings::setShotScale().
     * \par Cyclomatic complexity: 1
     */
    void setShotScale(qreal scale);

    /*!
     * \brief Forwards to Settings::shotCompression().
     * \par Cyclomatic complexity: 1
     */
    int shotCompression() const;
    /*!
     * \brief Forwards to Settings::setShotCompression().
     * \par Cyclomatic complexity: 1
     */
    void setShotCompression(int compression);

signals:
    void runningChanged();

    void usernameChanged();
    void passwordChanged();
    void autoStartupChanged();
    void postIntervalChanged();
    void enableShotsChanged();
    void shotScaleChanged();
    void shotCompressionChanged();

private:
    void setRunning(bool running);

    bool m_running = false;
    SendingService m_sendingService;
};
