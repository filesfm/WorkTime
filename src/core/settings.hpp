#pragma once

#include <QLoggingCategory>
#include <QObject>
#include <QSettings>
#include <QtClassHelperMacros>

Q_DECLARE_LOGGING_CATEGORY(worktimeSettings)

/*!
 * \brief Persisted application configuration, backed by QSettings.
 *
 * Settings is a process-wide singleton (see instance()). On construction,
 * any setting missing from the backing store, or holding a value outside
 * its documented invariant, is reset to its default (see settings.cpp).
 */
#if defined(BUILD_TESTING)
class Settings : public QObject
#else
class Settings final : public QObject
#endif
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Settings)

public:
    /*!
     * \brief Returns the process-wide instance, creating it on first call.
     * \par Cyclomatic complexity: 1
     */
    static Settings *instance();

#if defined(BUILD_TESTING)
    virtual ~Settings() = default;
#endif

    /*!
     * \brief Server account username. No constraint; may be empty.
     * \par Cyclomatic complexity: 1
     */
    QString username() const;
    /*!
     * \brief Sets username(). Emits usernameChanged() if the value changes.
     * \par Cyclomatic complexity: 2
     */
    void setUsername(const QString &username);

    /*!
     * \brief Server account password. No constraint; may be empty.
     * \par Cyclomatic complexity: 1
     */
    QString password() const;
    /*!
     * \brief Sets password(). Emits passwordChanged() if the value changes.
     * \par Cyclomatic complexity: 2
     */
    void setPassword(const QString &password);

    /*!
     * \brief Whether the app should launch automatically on system startup.
     * \par Cyclomatic complexity: 1
     */
    bool autoStartup() const;
    /*!
     * \brief Sets autoStartup(). Emits autoStartupChanged() if the value changes.
     * \par Cyclomatic complexity: 2
     */
    void setAutoStartup(bool enabled = true);

    /*!
     * \brief Whether the start button in main window pushed.
     * \par Cyclomatic complexity: 1
     */
    bool startButtonPushed() const;

    /*!
     * \brief Sets startButtonPushed(). Emits startButtonChanged() if the value changes.
     * \param Cyclomatic complexity: 2
     */
    void setStartButtonPushed(bool pushed = true);

signals:
    void usernameChanged();    //!< Emitted when username() changes.
    void passwordChanged();    //!< Emitted when password() changes.
    void autoStartupChanged(); //!< Emitted when autoStartup() changes.
    void startButtonChanged(); //!< Emitted when startButtonPushed() changes.

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    /*!
     * \brief Constructs the singleton, resetting any missing/invalid setting to its default.
     * \par Cyclomatic complexity: 4
     */
    explicit Settings(QObject *parent = nullptr);

    /*!
     * \brief Deletes unknown/invalid options and resets missing ones to their defaults.
     */
    void sanitize();

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    QSettings m_settings{QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime"};
};
