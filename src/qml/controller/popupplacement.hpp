#pragma once

#include <QObject>
#include <QString>
#include <QtClassHelperMacros>

/*!
 * \brief Lets the compositor place the tray popup next to the tray icon.
 *
 * On Wayland a client can't position its own window, so the popup would
 * otherwise land wherever the compositor chooses. Under GNOME Shell a bundled
 * shell extension moves it when it is shown; under KDE Plasma a bundled KWin
 * script does. Both place it next to the pointer, which sits on the tray icon
 * right after the icon is clicked, inside the work area of that screen.
 *
 * Elsewhere (macOS, Windows, X11) this does nothing, and the QML side
 * positions the popup on its own.
 */
class PopupPlacement : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupPlacement)

public:
    /*!
     * \brief Creates an inactive placer. Call start() to enable it.
     * \par Cyclomatic complexity: 1
     */
    explicit PopupPlacement(QObject *parent = nullptr);

    /*!
     * \brief Installs and enables the compositor-side placement for the
     * current desktop. Only has an effect on Wayland under GNOME or KDE.
     *
     * Call this only when a system tray icon is in use, since the popup is
     * the only window this placement is meant for.
     * \par Cyclomatic complexity: 4
     */
    void start();

    /*!
     * \brief Unloads the KWin script loaded by start(), if any. The GNOME
     * extension stays enabled, since it only acts when asked.
     * \par Cyclomatic complexity: 2
     */
    void stop();

    /*!
     * \brief Asks the GNOME extension to place the next popup window it sees.
     *
     * Call this before the popup is shown. Has no effect when the GNOME
     * extension isn't in use, since KWin places the popup on its own.
     * \par Cyclomatic complexity: 2
     */
    Q_INVOKABLE void requestPlacement() const;

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
#if defined(Q_OS_LINUX)
    //! \brief Whether the session is running on Wayland (real check, bypassable in tests).
    static bool isWayland();
    //! \brief Whether XDG_CURRENT_DESKTOP contains \a name.
    static bool desktopContains(const char *name);
    //! \brief Copies the bundled GNOME extension into the user's extensions directory.
    static bool installGnomeExtension();
    //! \brief Calls \a method ("EnableExtension"/"DisableExtension") on the GNOME extension manager.
    static bool callGnomeExtensionManager(const QString &method);

    //! \brief D-Bus destination callGnomeExtensionManager() talks to; overridable in tests.
    static QString s_gnomeShellExtensionsService;
    static QString s_gnomeShellExtensionsPath;
    //! \brief D-Bus destination requestPlacement() talks to; overridable in tests.
    static QString s_gnomePlacementService;
    static QString s_gnomePlacementPath;
    static QString s_gnomePlacementInterface;

#    if defined(BUILD_TESTING)
    //! \brief Overrides isWayland()'s result in tests, bypassing the real QPA platform check.
    static bool s_forceWaylandForTesting;
#    endif
#endif

private:
    //! \brief Whether the GNOME Shell extension is enabled for this session.
    bool m_gnomeActive = false;
    //! \brief Whether the KWin placement script is loaded.
    bool m_kwinActive = false;
};
