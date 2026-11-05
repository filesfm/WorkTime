#include "kwinscript.hpp"

#include "utilities.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

Q_LOGGING_CATEGORY(worktimeKWinScript, "worktime.kwin.script")

QString KWinScript::s_serviceName = QStringLiteral("org.kde.KWin");

QDBusInterface KWinScript::scriptingInterface()
{
    return QDBusInterface(s_serviceName,
                          QStringLiteral("/Scripting"),
                          QStringLiteral("org.kde.kwin.Scripting"),
                          QDBusConnection::sessionBus());
}

bool KWinScript::load(const QString &pluginName, const QString &resourcePath, const QString &fileName)
{
    QDBusInterface scripting = scriptingInterface();
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

    QDBusInterface(s_serviceName,
                   QStringLiteral("/Scripting/Script%1").arg(scriptId.value()),
                   QStringLiteral("org.kde.kwin.Script"),
                   QDBusConnection::sessionBus())
        .call(QStringLiteral("run"));

    return true;
}

void KWinScript::unload(const QString &pluginName)
{
    if (QDBusInterface scripting = scriptingInterface(); scripting.isValid())
        scripting.call(QStringLiteral("unloadScript"), pluginName);
}
