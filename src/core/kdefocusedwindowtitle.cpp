#include "kdefocusedwindowtitle.hpp"

#include "kwinscript.hpp"

#include <QDBusConnection>

Q_LOGGING_CATEGORY(worktimeKdeFocusedWindowTitle, "worktime.kde.focused.window.title")

namespace {

constexpr QLatin1StringView kServiceName{"io.github.filesfm.worktime"};
constexpr QLatin1StringView kObjectPath{"/io/github/filesfm/worktime/FocusedWindow"};

constexpr QLatin1StringView kKWinPluginName{"io.github.filesfm.worktime.focusedwindow"};

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
    if (m_started) {
        qCDebug(worktimeKdeFocusedWindowTitle) << "start called while already started, ignoring";
        return;
    }

    qCInfo(worktimeKdeFocusedWindowTitle) << "starting";

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(kServiceName)) {
        qCWarning(worktimeKdeFocusedWindowTitle) << "failed to register D-Bus service" << kServiceName;
        return;
    }
    bus.registerObject(kObjectPath, this, QDBusConnection::ExportScriptableSlots);

    if (!KWinScript::load(QString(kKWinPluginName),
                          QStringLiteral(":/qt/qml/Worktime/focused-window-title.js"),
                          QStringLiteral("worktime-focused-window-title.js"))) {
        return;
    }

    m_started = true;
}

void KDEFocusedWindowTitle::stop()
{
    if (!m_started) {
        qCDebug(worktimeKdeFocusedWindowTitle) << "stop called while not started, ignoring";
        return;
    }

    qCInfo(worktimeKdeFocusedWindowTitle) << "stopping";

    KWinScript::unload(QString(kKWinPluginName));

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(kObjectPath);
    bus.unregisterService(kServiceName);

    m_started = false;
}

QString KDEFocusedWindowTitle::activeWindowTitle() const
{
    QMutexLocker locker(&m_activeWindowTitleMutex);
    return m_activeWindowTitle;
}

void KDEFocusedWindowTitle::setActiveWindowTitle(const QString &title)
{
    qCDebug(worktimeKdeFocusedWindowTitle) << "active window title changed to" << title;
    QMutexLocker locker(&m_activeWindowTitleMutex);
    m_activeWindowTitle = title;
}