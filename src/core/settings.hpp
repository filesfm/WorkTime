#pragma once

#include <QObject>
#include <QSettings>
#include <QtClassHelperMacros>

/*!
 * \brief Persisted application configuration, backed by QSettings.
 *
 * Settings is a process-wide singleton (see instance()). On construction,
 * any setting missing from the backing store, or holding a value outside
 * its documented invariant, is reset to its default (see settings.cpp).
 */
class Settings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Settings)

public:
    /*! \brief Returns the process-wide instance, creating it on first call. */
    static Settings *instance();

    /*! \brief Server account username. No constraint; may be empty. */
    QString username() const;
    /*! \brief Sets username(). Emits usernameChanged() if the value changes. */
    void setUsername(const QString &username);

    /*! \brief Server account password. No constraint; may be empty. */
    QString password() const;
    /*! \brief Sets password(). Emits passwordChanged() if the value changes. */
    void setPassword(const QString &password);

    /*! \brief Whether the app should launch automatically on system startup. */
    bool autoStartup() const;
    /*! \brief Sets autoStartup(). Emits autoStartupChanged() if the value changes. */
    void setAutoStartup(bool enabled = true);

    /*! \brief Interval, in seconds, between tracked-time submissions. Always > 0. */
    int postInterval() const;
    /*!
     * \brief Sets postInterval().
     * \param seconds Must be > 0 to satisfy the stored invariant.
     * Emits postIntervalChanged() if the value changes.
     */
    void setPostInterval(int seconds);

    /*! \brief Whether periodic screenshots are captured while tracking. */
    bool enableShots() const;
    /*! \brief Sets enableShots(). Emits enableShotsChanged() if the value changes. */
    void setEnableShots(bool enabled = true);

    /*! \brief Screenshot downscale factor, as a fraction of full resolution in (0.0, 1.0]. */
    qreal shotScale() const;
    /*!
     * \brief Sets shotScale().
     * \param scale Must be in (0.0, 1.0] to satisfy the stored invariant.
     * Emits shotScaleChanged() if the value changes.
     */
    void setShotScale(qreal scale);

    /*! \brief Screenshot compression/quality percentage, in [0, 100]. */
    int shotCompression() const;
    /*!
     * \brief Sets shotCompression().
     * \param compression Must be in [0, 100] to satisfy the stored invariant.
     * Emits shotCompressionChanged() if the value changes.
     */
    void setShotCompression(int compression);

signals:
    void usernameChanged();        //!< Emitted when username() changes.
    void passwordChanged();        //!< Emitted when password() changes.
    void autoStartupChanged();     //!< Emitted when autoStartup() changes.
    void postIntervalChanged();    //!< Emitted when postInterval() changes.
    void enableShotsChanged();     //!< Emitted when enableShots() changes.
    void shotScaleChanged();       //!< Emitted when shotScale() changes.
    void shotCompressionChanged(); //!< Emitted when shotCompression() changes.

private:
    explicit Settings(QObject *parent = nullptr);

    QSettings m_settings;
};
