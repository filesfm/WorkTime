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
    explicit MainController(QObject *parent = nullptr);

    bool running() const;

    /*!
     * \brief Starts tracking if stopped, stops it if running.
     * Emits runningChanged() if the state changes.
     */
    Q_INVOKABLE void toggleTracking();

    /*! \brief Forwards to Settings::username(). */
    QString username() const;
    /*! \brief Forwards to Settings::setUsername(). */
    void setUsername(const QString &username);

    /*! \brief Forwards to Settings::password(). */
    QString password() const;
    /*! \brief Forwards to Settings::setPassword(). */
    void setPassword(const QString &password);

    /*! \brief Forwards to Settings::autoStartup(). */
    bool autoStartup() const;
    /*! \brief Forwards to Settings::setAutoStartup(). */
    void setAutoStartup(bool enabled);

    /*! \brief Forwards to Settings::postInterval(). */
    int postInterval() const;
    /*! \brief Forwards to Settings::setPostInterval(). */
    void setPostInterval(int seconds);

    /*! \brief Forwards to Settings::enableShots(). */
    bool enableShots() const;
    /*! \brief Forwards to Settings::setEnableShots(). */
    void setEnableShots(bool enabled);

    /*! \brief Forwards to Settings::shotScale(). */
    qreal shotScale() const;
    /*! \brief Forwards to Settings::setShotScale(). */
    void setShotScale(qreal scale);

    /*! \brief Forwards to Settings::shotCompression(). */
    int shotCompression() const;
    /*! \brief Forwards to Settings::setShotCompression(). */
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
