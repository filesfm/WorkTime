#pragma once

#include <QImage>
#include <QList>
#include <QLoggingCategory>
#include <QString>
#include <QUrl>

Q_DECLARE_LOGGING_CATEGORY(worktimeUtilities)

/*!
 * \brief Stateless OS-interaction helpers
 */
class Utilities
{
public:
    /*! \brief Deleted: Utilities is never instantiated, only used via its static members. */
    Utilities() = delete;

    /*!
     * \brief Title of the currently focused window.
     *
     * \return The focused window's title, or an empty string if it could
     * not be determined.
     * \par Cyclomatic complexity: 7 (Linux), 5 (Windows), 2 (macOS), 1 (other platforms) - the
     * implementation differs per platform (see utilities.cpp / utilities_mac.mm).
     */
    static QString focusedWindowTitle();

    /*!
     * \brief Copies a file (e.g. a `:/` Qt resource) to a fixed path under
     * the system temp directory, overwriting any file already there.
     *
     * Needed because some platform APIs (e.g. Qt.labs.platform's
     * SystemTrayIcon on Linux) can't load an icon straight out of the Qt
     * resource system and need a plain file:// path instead.
     *
     * \param sourcePath Path to copy from, as accepted by QFile (a `:/...`
     * resource path or a path on disk).
     * \param fileName Destination file name, placed directly under
     * QStandardPaths::TempLocation.
     * \return A file:// URL pointing at the destination path.
     * \par Cyclomatic complexity: 1
     */
    static QUrl extractResourceToDisk(const QString &sourcePath, const QString &fileName);

    /*!
     * \brief Truncates \a value to at most \a maxCodePoints Unicode code points.
     *
     * Truncation happens at a code-point boundary rather than a UTF-16 code
     * unit boundary, so a surrogate pair (a character outside the Basic
     * Multilingual Plane, e.g. an emoji) is never split - which in turn
     * guarantees the UTF-8 encoding of the result is never split either.
     *
     * \return \a value unchanged if it already has at most \a maxCodePoints
     * code points, otherwise the first \a maxCodePoints of them.
     * \par Cyclomatic complexity: 2
     */
    static QString truncateUtf8Safe(const QString &value, qsizetype maxCodePoints);

    /*!
     * \brief Whether the OS is currently configured to automatically sync its
     * clock (e.g. via NTP), rather than relying on a wall clock that could
     * have been set to anything.
     *
     * Checked because activity timestamps sent to the server come straight
     * from the local wall clock (QDateTime::currentSecsSinceEpoch(), see
     * SendingService::sendNow()) with no server-side validation: a wrong OS
     * clock means wrong tracked-time data, silently.
     *
     * Fails open (returns \c true) whenever the underlying check itself is
     * inconclusive - a D-Bus error, a missing registry key, a failed shell
     * command. A false "enabled" costs nothing beyond today's status quo
     * (silence); a false "disabled" costs an actionable-looking notification
     * the user may have no way to act on - the second is worse, so ties go
     * to "enabled".
     *
     * \return \c true if automatic clock sync is on (or the check couldn't
     * tell), \c false if it's confirmed off. Backed by the `NTP` property of
     * `org.freedesktop.timedate1` over the D-Bus system bus (Linux), the
     * W32Time `Parameters\Type` registry value (Windows), or
     * the `TMAutomaticTimeEnabled` reply of the `timed` daemon over the
     * `com.apple.timed.xpc` XPC service (macOS).
     * \par Cyclomatic complexity: 5 (Linux), 3 (Windows), 3 (macOS).
     */
    static bool isClockSyncEnabled();

#if !defined(BUILD_WITHOUT_AUTOSTART)
    //! \brief Registers or unregisters the application to launch at login.
    static void autostart(bool autostart = true);

    /*!
     * \brief Whether the OS is actually set up to launch the application at login.
     *
     * Reads the real OS state rather than the stored setting: the Run registry
     * entry on Windows, the SMAppService status on macOS and the autostart
     * desktop entry written by the Background portal on Linux.
     */
    static bool isAutostartEnabled();
#endif

#if defined(Q_OS_LINUX)
    //! \brief Whether the GNOME focused-window D-Bus extension is installed.
    static bool isGNOMEFocusedWindowDBusInstalled();

    //! \brief Whether the GNOME focused-window D-Bus extension is enabled.
    static bool isGNOMEFocusedWindowDBusEnabled();

    //! \brief Installs the GNOME focused-window D-Bus extension.
    static void installGNOMEFocusedWindowDBus();

    //! \brief Enables the GNOME focused-window D-Bus extension.
    static void enableGNOMEFocusedWindowDBus();
#endif

    /*!
     * \brief Shows a desktop notification with \a title and \a message.
     *
     * \par Cyclomatic complexity: 1 (Linux), 5 (macOS).
     */
    static void showNotification(const QString &title, const QString &message);

#if !defined(BUILD_WITHOUT_AUTOSTART)
#    if defined(BUILD_TESTING)
protected:
#    else
private:
#    endif
#    if defined(Q_OS_LINUX)
    //! \brief D-Bus service autostart() sends the Background portal request to.
    static QString s_autostartPortalService;
    //! \brief Directory isAutostartEnabled() looks for the autostart desktop entry in.
    static QString s_autostartDirectory;
#    elif defined(Q_OS_WINDOWS)
    //! \brief Registry key autostart() writes the launch-at-login entry to.
    static QString s_autostartRegistryKey;
#    elif defined(Q_OS_MACOS)
    /*!
     * \brief Object autostart() registers/unregisters instead of
     * `[SMAppService mainAppService]`, or null to use the latter. Must respond
     * to `registerAndReturnError:`, `unregisterAndReturnError:` and `status`. Typed
     * `void *` so this header stays includable from plain C++.
     */
    static void *s_autostartAppService;
#    endif
#endif

#if defined(Q_OS_LINUX)
#    if defined(BUILD_TESTING)
protected:
#    else
private:
#    endif
    //! \brief D-Bus destination isGNOMEFocusedWindowDBus*()/installGNOMEFocusedWindowDBus()/enableGNOMEFocusedWindowDBus() talk to; overridable in tests.
    static QString s_gnomeExtensionsService;
    //! \brief D-Bus destination showNotification() talks to; overridable in tests.
    static QString s_notificationsService;
    //! \brief D-Bus destination focusedWindowTitle() talks to under GNOME; overridable in tests.
    static QString s_gnomeShellService;
    //! \brief D-Bus destination focusedWindowTitle() talks to under Cinnamon; overridable in tests.
    static QString s_cinnamonService;
#endif

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
#if defined(Q_OS_LINUX)
    //! \brief D-Bus destination isClockSyncEnabled() talks to on the system bus; overridable in tests.
    static QString s_clockSyncService;
#elif defined(Q_OS_WINDOWS)
    //! \brief Registry key isClockSyncEnabled() reads the NTP-enabled toggle from.
    static QString s_clockSyncRegistryKey;
#elif defined(Q_OS_MACOS)
    //! \brief Mach service isClockSyncEnabled() asks for the automatic-time toggle.
    static const char *s_clockSyncMachService;
#endif
};
