#include "kdefocusedwindowtitle.hpp"

#include "utilities.hpp"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

namespace {

constexpr QLatin1StringView kServiceName{"io.github.filesfm.worktime"};
constexpr QLatin1StringView kObjectPath{"/io/github/filesfm/worktime/FocusedWindow"};

constexpr QLatin1StringView kKWinPluginName{"io.github.filesfm.worktime.focusedwindow"};

QDBusInterface kwinScriptingInterface()
{
    return QDBusInterface(QStringLiteral("org.kde.KWin"),
                          QStringLiteral("/Scripting"),
                          QStringLiteral("org.kde.kwin.Scripting"),
                          QDBusConnection::sessionBus());
}

} // namespace

KDEFocusedWindowTitle *KDEFocusedWindowTitle::instance()
{
    static KDEFocusedWindowTitle instance;
    return &instance;
}

KDEFocusedWindowTitle::KDEFocusedWindowTitle(QObject *parent)
    : QObject(parent)
{}

void KDEFocusedWindowTitle::start()
{
    if (m_started)
        return;

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(kServiceName))
        return;
    bus.registerObject(kObjectPath, this, QDBusConnection::ExportScriptableSlots);

    QDBusInterface scripting = kwinScriptingInterface();
    if (!scripting.isValid())
        return;

    const QString pluginName(kKWinPluginName);
    const QDBusReply<bool> alreadyLoaded = scripting.call(QStringLiteral("isScriptLoaded"), pluginName);
    if (alreadyLoaded.isValid() && alreadyLoaded.value())
        scripting.call(QStringLiteral("unloadScript"), pluginName);

    const QString scriptPath = Utilities::extractResourceToDisk(QStringLiteral(
                                                                    ":/qt/qml/Worktime/focused-window-title.js"),
                                                                QStringLiteral("worktime-focused-window-title.js"))
                                   .toLocalFile();

    const QDBusReply<int> scriptId = scripting.call(QStringLiteral("loadScript"), scriptPath, pluginName);
    if (!scriptId.isValid() || scriptId.value() < 0)
        return;

    QDBusInterface(QStringLiteral("org.kde.KWin"),
                   QStringLiteral("/Scripting/Script%1").arg(scriptId.value()),
                   QStringLiteral("org.kde.kwin.Scripting"),
                   bus)
        .call(QStringLiteral("run"));

    m_started = true;
}

void KDEFocusedWindowTitle::stop()
{
    if (!m_started)
        return;

    QDBusInterface scripting = kwinScriptingInterface();
    if (scripting.isValid())
        scripting.call(QStringLiteral("unloadScript"), QString(kKWinPluginName));

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(kObjectPath);
    bus.unregisterService(kServiceName);

    m_started = false;
}

QString KDEFocusedWindowTitle::activeWindowTitle() const
{
    return m_activeWindowTitle;
}

void KDEFocusedWindowTitle::setActiveWindowTitle(const QString &title)
{
    m_activeWindowTitle = title;
}