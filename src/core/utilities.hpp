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

    //! \brief Registers or unregisters the application to launch at login.
    static void autostart(bool autostart = true);

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
};
