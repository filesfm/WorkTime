#include "kwinscript.hpp"

#include "utilities.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

Q_LOGGING_CATEGORY(worktimeKWinScript, "worktime.kwin.script")

namespace {

QDBusInterface kwinScriptingInterface()
{
    return QDBusInterface(QStringLiteral("org.kde.KWin"),
                          QStringLiteral("/Scripting"),
                          QStringLiteral("org.kde.kwin.Scripting"),
                          QDBusConnection::sessionBus());
}

} // namespace

namespace KWinScript {

bool load(const QString &pluginName, const QString &resourcePath, const QString &fileName)
{
    QDBusInterface scripting = kwinScriptingInterface();
    if (!scripting.isValid()) {
        qCWarning(worktimeKWinScript) << "org.kde.KWin Scripting interface unavailable";
        return false;
    }

    if (const QDBusReply<bool> alreadyLoaded = scripting.call(QStringLiteral("isScriptLoaded"), pluginName);
        alreadyLoaded.isValid() && alreadyLoaded.value()) {
        qCDebug(worktimeKWinScript) << "unloading already-loaded KWin script" << pluginName;
        scripting.call(QStringLiteral("unloadScript"), pluginName);
    }

    const QString scriptPath = Utilities::extractResourceToDisk(resourcePath, fileName).toLocalFile();

    const QDBusReply<int> scriptId = scripting.call(QStringLiteral("loadScript"), scriptPath, pluginName);
    if (!scriptId.isValid() || scriptId.value() < 0) {
        qCWarning(worktimeKWinScript) << "failed to load KWin script" << scriptPath << ":"
                                      << scriptId.error().message();
        return false;
    }

    QDBusInterface(QStringLiteral("org.kde.KWin"),
                   QStringLiteral("/Scripting/Script%1").arg(scriptId.value()),
                   QStringLiteral("org.kde.kwin.Script"),
                   QDBusConnection::sessionBus())
        .call(QStringLiteral("run"));

    return true;
}

void unload(const QString &pluginName)
{
    if (QDBusInterface scripting = kwinScriptingInterface(); scripting.isValid())
        scripting.call(QStringLiteral("unloadScript"), pluginName);
}

} // namespace KWinScript
