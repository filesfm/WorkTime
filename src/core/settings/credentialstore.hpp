#pragma once

#include <QLoggingCategory>
#include <QString>

#if defined(Q_OS_LINUX)
#    include <QDBusObjectPath>

class QDBusInterface;
#endif

Q_DECLARE_LOGGING_CATEGORY(worktimeCredentialStore)

/*!
 * \brief Stores the server account password in the OS credential store
 * instead of in Settings' plaintext QSettings backing file - Secret
 * Service (GNOME Keyring/KWallet) on Linux, Keychain Services on macOS,
 * Credential Manager on Windows.
 *
 * There is exactly one entry, since Settings only ever holds one configured
 * account at a time; it is independent of username() so renaming the
 * account doesn't orphan or require migrating a credential-store entry.
 * \note Thread-safety: Thread-safe.
 */
#if defined(BUILD_TESTING)
class CredentialStore
#else
class CredentialStore final
#endif
{
public:
    /*! \brief Deleted: CredentialStore is never instantiated, only used via its static members. */
    CredentialStore() = delete;

    /*!
     * \return The stored password, or an empty string if none is stored or
     * the credential store could not be reached.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: platform-dependent.
     */
    static QString readPassword();

    /*!
     * \brief Stores \a password, overwriting any existing entry.
     * \return Whether the write succeeded.
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: platform-dependent.
     */
    static bool writePassword(const QString &password);

    /*!
     * \brief Deletes the stored password, if any.
     * \return Whether the entry is now absent (true if deleted, or if there
     * was nothing to delete).
     * \note Thread-safety: Thread-safe.
     * \par Cyclomatic complexity: platform-dependent.
     */
    static bool deletePassword();

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
#if defined(Q_OS_LINUX)
    //! \brief Interface to the Secret Service object at s_secretServicePath, on s_secretServiceName.
    static QDBusInterface serviceInterface();
    //! \brief Blocks until the prompt at promptPath completes; a no-op for the well-known "/" (no prompt needed).
    static void waitForPrompt(const QDBusObjectPath &promptPath);
    //! \brief Opens a "plain" Secret Service session.
    static QDBusObjectPath openSession();
    //! \brief Finds this app's stored item, unlocking it first if needed.
    static QDBusObjectPath findItem();

    //! \brief D-Bus destination every call above talks to; overridable in tests.
    static QString s_secretServiceName;
    //! \brief Path of the Secret Service's root object; overridable in tests.
    static QString s_secretServicePath;
    //! \brief Path writePassword() creates its item in; overridable in tests.
    static QString s_defaultCollectionPath;
#endif
};
