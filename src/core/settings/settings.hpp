#pragma once

#include <QLoggingCategory>
#include <QObject>
#include <QSettings>
#include <QtClassHelperMacros>

Q_DECLARE_LOGGING_CATEGORY(worktimeSettings)

/*!
 * \brief Persisted application configuration, backed by QSettings.
 * \note Thread-safety: Thread-safe.
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
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 1
     */
    static Settings *instance();

#if defined(BUILD_TESTING)
    virtual ~Settings() = default;
#endif

    /*!
     * \brief Server account username. No constraint; may be empty.
     * \note Thread-safety: Thread-safe and reentrant.
     * \par Cyclomatic complexity: 1
     */
    QString username() const;
    /*!
     * \brief Sets username(). Emits usernameChanged() if the value changes.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 2
     */
    void setUsername(const QString &username);

    /*!
     * \brief Server account password. No constraint; may be empty. Held in
     * the OS credential store (see CredentialStore), not in this class's
     * plaintext QSettings backing file; this getter returns an in-memory
     * copy cached by loadPassword(), so it stays cheap to call.
     * \note Thread-safety: Thread-safe and reentrant.
     * \par Cyclomatic complexity: 1
     */
    QString password() const;
    /*!
     * \brief Sets password(), writing it through to the OS credential store.
     * Emits passwordChanged() if the value changes.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 2
     */
    void setPassword(const QString &password);

#if !defined(BUILD_WITHOUT_AUTOSTART)
    /*!
     * \brief Whether the app should launch automatically on system startup.
     * \note Thread-safety: Thread-safe and reentrant.
     * \par Cyclomatic complexity: 1
     */
    bool autoStartup() const;
    /*!
     * \brief Sets autoStartup(). Emits autoStartupChanged() if the value changes.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 2
     */
    void setAutoStartup(bool enabled = true);
#endif

    /*!
     * \brief Whether the start button in main window pushed.
     * \note Thread-safety: Thread-safe and reentrant.
     * \par Cyclomatic complexity: 1
     */
    bool startButtonPushed() const;

    /*!
     * \brief Sets startButtonPushed(). Emits startButtonChanged() if the value changes.
     * \note Thread-safety: Thread-safe.
     * \param Cyclomatic complexity: 2
     */
    void setStartButtonPushed(bool pushed = true);

signals:
    void usernameChanged(); //!< Emitted when username() changes.
    void passwordChanged(); //!< Emitted when password() changes.
#if !defined(BUILD_WITHOUT_AUTOSTART)
    void autoStartupChanged(); //!< Emitted when autoStartup() changes.
#endif
    void startButtonChanged(); //!< Emitted when startButtonPushed() changes.

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    /*!
     * \brief Constructs the singleton, resetting any missing/invalid setting to its default.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: 4
     */
    explicit Settings(QObject *parent = nullptr);

    /*!
     * \brief Deletes unknown/invalid options and resets missing ones to their defaults.
     * \note Thread-safety: Thread-safe.
     */
    void sanitize();

    /*!
     * \brief Loads m_cachedPassword from CredentialStore::readPassword(). If
     * the credential store is empty but a legacy plaintext password is
     * still present in m_settings (from before CredentialStore existed),
     * migrates it: writes it to the credential store and removes it from
     * m_settings, or - if the write fails - keeps it in m_settings and
     * retries the migration on the next launch.
     * \par Cyclomatic complexity: 4
     */
    void loadPassword();

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    QSettings m_settings{QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime"};
    /*! \brief In-memory copy of the password held in the OS credential store; see password()/loadPassword(). */
    QString m_cachedPassword;
};
