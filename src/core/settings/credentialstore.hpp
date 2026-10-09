#pragma once

#include <QLoggingCategory>
#include <QString>

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
class CredentialStore
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
};
