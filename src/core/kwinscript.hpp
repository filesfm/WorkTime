#pragma once

#include <QLoggingCategory>
#include <QString>

class QDBusInterface;

Q_DECLARE_LOGGING_CATEGORY(worktimeKWinScript)

/*!
 * \brief Loads and unloads KWin scripts bundled as Qt resources, under KDE Plasma.
 *
 * A script is identified by its plugin name, which is how KWin tells apart
 * the scripts this app has loaded.
 */
#if defined(BUILD_TESTING)
class KWinScript
#else
class KWinScript final
#endif
{
public:
    /*! \brief Deleted: KWinScript is never instantiated, only used via its static members. */
    KWinScript() = delete;

    /*!
     * \brief Extracts the script at \a resourcePath to disk and runs it in KWin
     * under \a pluginName, replacing any copy of it that is already loaded.
     *
     * \param resourcePath Qt resource path of the script, e.g. `:/qt/qml/...`.
     * \param fileName Name the extracted script is saved as in the temp directory.
     * \return Whether the script is now running.
     * \par Cyclomatic complexity: 4
     */
    static bool load(const QString &pluginName, const QString &resourcePath, const QString &fileName);

    /*!
     * \brief Unloads the script registered under \a pluginName. No-op if KWin is
     * unreachable or the script is not loaded.
     * \par Cyclomatic complexity: 1
     */
    static void unload(const QString &pluginName);

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    //! \brief Interface to org.kde.KWin's /Scripting object, at s_serviceName.
    static QDBusInterface scriptingInterface();

    //! \brief D-Bus service load()/unload() talk to; overridable in tests.
    static QString s_serviceName;
};
