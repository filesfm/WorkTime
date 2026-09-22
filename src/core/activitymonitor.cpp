#include "activitymonitor.hpp"

#include <QSocketNotifier>

Q_LOGGING_CATEGORY(worktimeActivityMonitor, "worktime.activity.monitor")

ActivityMonitor::ActivityMonitor(QObject *parent)
    : QObject(parent)
{}

ActivityMonitor::~ActivityMonitor()
{
    stop();
}

#if defined(Q_OS_LINUX)

#    include <cerrno>
#    include <cstring>
#    include <fcntl.h>
#    include <linux/input.h>
#    include <unistd.h>

#    include <QDBusConnection>
#    include <QDBusInterface>
#    include <QDBusReply>

extern "C" {
#    include <libudev.h>
}

namespace {
bool isKeyboardOrMouse(udev_device *device)
{
    const char *isKeyboard = udev_device_get_property_value(device, "ID_INPUT_KEYBOARD");
    const char *isMouse = udev_device_get_property_value(device, "ID_INPUT_MOUSE");
    return (isKeyboard && QLatin1String(isKeyboard) == "1") || (isMouse && QLatin1String(isMouse) == "1");
}

constexpr QLatin1StringView kGnomeIdleMonitorService{"org.gnome.Mutter.IdleMonitor"};
constexpr QLatin1StringView kGnomeIdleMonitorPath{"/org/gnome/Mutter/IdleMonitor/Core"};
constexpr QLatin1StringView kGnomeIdleMonitorInterface{"org.gnome.Mutter.IdleMonitor"};
} // namespace

bool ActivityMonitor::startGnomeIdleMonitor()
{
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP") != "GNOME")
        return false;

    m_gnomeIdleMonitor = new QDBusInterface(QString(kGnomeIdleMonitorService),
                                            QString(kGnomeIdleMonitorPath),
                                            QString(kGnomeIdleMonitorInterface),
                                            QDBusConnection::sessionBus(),
                                            this);
    if (!m_gnomeIdleMonitor->isValid()) {
        qCWarning(worktimeActivityMonitor) << "org.gnome.Mutter.IdleMonitor unavailable, falling back to /dev/input";
        delete m_gnomeIdleMonitor;
        m_gnomeIdleMonitor = nullptr;
        return false;
    }

    QDBusConnection::sessionBus().connect(QString(kGnomeIdleMonitorService),
                                          QString(kGnomeIdleMonitorPath),
                                          QString(kGnomeIdleMonitorInterface),
                                          QStringLiteral("WatchFired"),
                                          this,
                                          SLOT(handleGnomeIdleWatchFired(uint)));

    qCInfo(worktimeActivityMonitor) << "using org.gnome.Mutter.IdleMonitor for activity detection";
    armGnomeUserActiveWatch();
    return true;
}

void ActivityMonitor::stopGnomeIdleMonitor()
{
    QDBusConnection::sessionBus().disconnect(QString(kGnomeIdleMonitorService),
                                             QString(kGnomeIdleMonitorPath),
                                             QString(kGnomeIdleMonitorInterface),
                                             QStringLiteral("WatchFired"),
                                             this,
                                             SLOT(handleGnomeIdleWatchFired(uint)));

    if (m_gnomeIdleMonitor && m_gnomeWatchId != 0)
        m_gnomeIdleMonitor->call(QStringLiteral("RemoveWatch"), m_gnomeWatchId);

    delete m_gnomeIdleMonitor;
    m_gnomeIdleMonitor = nullptr;
    m_gnomeWatchId = 0;
}

void ActivityMonitor::armGnomeUserActiveWatch()
{
    if (!m_gnomeIdleMonitor)
        return;

    const QDBusReply<uint> reply = m_gnomeIdleMonitor->call(QStringLiteral("AddUserActiveWatch"));
    m_gnomeWatchId = reply.isValid() ? reply.value() : 0;
}

void ActivityMonitor::handleGnomeIdleWatchFired(uint watchId)
{
    if (watchId != m_gnomeWatchId)
        return;

    // AddUserActiveWatch() is one-shot: fires once on the next input event,
    // then must be re-added to keep receiving notifications.
    emit activityDetected();
    armGnomeUserActiveWatch();
}

void ActivityMonitor::start()
{
    if (m_running)
        return;

    qCInfo(worktimeActivityMonitor) << "starting activity monitor";

    if (startGnomeIdleMonitor()) {
        m_usingGnomeIdleMonitor = true;
        m_running = true;
        return;
    }

    m_udev = udev_new();
    if (!m_udev) {
        qCCritical(worktimeActivityMonitor) << "failed to create udev context";
        return;
    }

    m_udevMonitor = udev_monitor_new_from_netlink(m_udev, "udev");
    if (!m_udevMonitor) {
        qCCritical(worktimeActivityMonitor) << "failed to create udev monitor";
        udev_unref(m_udev);
        m_udev = nullptr;
        return;
    }

    udev_monitor_filter_add_match_subsystem_devtype(m_udevMonitor, "input", nullptr);
    udev_monitor_enable_receiving(m_udevMonitor);

    m_udevMonitorNotifier = new QSocketNotifier(udev_monitor_get_fd(m_udevMonitor), QSocketNotifier::Read, this);
    connect(m_udevMonitorNotifier, &QSocketNotifier::activated, this, &ActivityMonitor::readUdevMonitor);

    scanInputDevices();
    m_running = true;
}

void ActivityMonitor::stop()
{
    if (!m_running)
        return;

    qCInfo(worktimeActivityMonitor) << "stopping activity monitor";

    if (m_usingGnomeIdleMonitor) {
        stopGnomeIdleMonitor();
        m_usingGnomeIdleMonitor = false;
        m_running = false;
        return;
    }

    const QStringList devNodes = m_deviceNotifiers.keys();
    for (const QString &devNode : devNodes)
        removeInputDevice(devNode);

    delete m_udevMonitorNotifier;
    m_udevMonitorNotifier = nullptr;

    if (m_udevMonitor)
        udev_monitor_unref(m_udevMonitor);
    m_udevMonitor = nullptr;

    if (m_udev)
        udev_unref(m_udev);
    m_udev = nullptr;

    m_running = false;
}

void ActivityMonitor::scanInputDevices()
{
    udev_enumerate *enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);

    udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry *entry;
    udev_list_entry_foreach(entry, devices)
    {
        udev_device *device = udev_device_new_from_syspath(m_udev, udev_list_entry_get_name(entry));
        if (!device)
            continue;

        const char *devNode = udev_device_get_devnode(device);
        if (devNode && isKeyboardOrMouse(device))
            addInputDevice(QString::fromUtf8(devNode));

        udev_device_unref(device);
    }

    udev_enumerate_unref(enumerate);
}

void ActivityMonitor::addInputDevice(const QString &devNode)
{
    if (m_deviceNotifiers.contains(devNode))
        return;

    const int fd = ::open(devNode.toUtf8().constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        qCWarning(worktimeActivityMonitor) << "failed to open" << devNode << ":" << std::strerror(errno);
        return;
    }

    qCDebug(worktimeActivityMonitor) << "watching input device" << devNode;

    auto *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, [this, fd]() { readInputDevice(fd); });
    m_deviceNotifiers.insert(devNode, notifier);
}

void ActivityMonitor::removeInputDevice(const QString &devNode)
{
    QSocketNotifier *notifier = m_deviceNotifiers.take(devNode);
    if (!notifier)
        return;

    qCDebug(worktimeActivityMonitor) << "no longer watching input device" << devNode;

    const int fd = static_cast<int>(notifier->socket());
    delete notifier;
    ::close(fd);
}

void ActivityMonitor::readInputDevice(int fd)
{
    struct input_event event{};
    bool activity = false;

    while (::read(fd, &event, sizeof(event)) == sizeof(event)) {
        if (event.type == EV_KEY || event.type == EV_REL || event.type == EV_ABS)
            activity = true;
    }

    if (activity)
        emit activityDetected();
}

void ActivityMonitor::readUdevMonitor()
{
    udev_device *device = udev_monitor_receive_device(m_udevMonitor);
    if (!device)
        return;

    const char *action = udev_device_get_action(device);
    const char *devNode = udev_device_get_devnode(device);

    if (action && devNode) {
        const QString devNodeStr = QString::fromUtf8(devNode);

        if (QLatin1String(action) == "remove") {
            removeInputDevice(devNodeStr);
        } else if (QLatin1String(action) == "add" && isKeyboardOrMouse(device)) {
            qCDebug(worktimeActivityMonitor) << "detected new keyboard/mouse device" << devNodeStr;
            addInputDevice(devNodeStr);
        }
    }

    udev_device_unref(device);
}

#elif defined(Q_OS_WIN)

#    include <windows.h>

namespace {
ActivityMonitor *g_activityMonitor = nullptr;

LRESULT CALLBACK lowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_activityMonitor)
        emit g_activityMonitor->activityDetected();

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

LRESULT CALLBACK lowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION && g_activityMonitor)
        emit g_activityMonitor->activityDetected();

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}
} // namespace

void ActivityMonitor::start()
{
    if (m_running)
        return;

    g_activityMonitor = this;
    m_keyboardHook = SetWindowsHookExA(WH_KEYBOARD_LL, lowLevelKeyboardProc, nullptr, 0);
    m_mouseHook = SetWindowsHookExA(WH_MOUSE_LL, lowLevelMouseProc, nullptr, 0);
    m_running = true;
}

void ActivityMonitor::stop()
{
    if (!m_running)
        return;

    if (m_keyboardHook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_keyboardHook));
        m_keyboardHook = nullptr;
    }
    if (m_mouseHook) {
        UnhookWindowsHookEx(static_cast<HHOOK>(m_mouseHook));
        m_mouseHook = nullptr;
    }

    g_activityMonitor = nullptr;
    m_running = false;
}

#elif defined(Q_OS_MACOS)

// Implemented in activitymonitor_mac.mm

#endif
