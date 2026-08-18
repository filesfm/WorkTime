#include "utilities.hpp"

#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>

QList<QImage> Utilities::captureScreenshots()
{
    QList<QImage> images;
    for (QScreen *screen : QGuiApplication::screens())
        images.append(screen->grabWindow(0).toImage());

    return images;
}

#if defined(Q_OS_LINUX)

#    include <QFile>

#    include <optional>

#    include <X11/Xatom.h>
#    include <X11/Xlib.h>

namespace {

// Reads a window property expected to hold a single 32-bit value of type
// `type` (e.g. a Window ID or a PID), returning std::nullopt if the
// property is absent, of a different type, or empty. Per Xlib convention,
// 32-bit property values are delivered as an array of `long`s regardless of
// the platform's actual long width.
template<typename T>
std::optional<T> readProperty(Display *display, Window window, Atom property, Atom type)
{
    Atom actualType;
    int actualFormat = 0;
    unsigned long itemCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;

    const int status = XGetWindowProperty(
        display, window, property, 0, 1, False, type, &actualType, &actualFormat, &itemCount, &bytesAfter, &data);

    if (status != Success || !data)
        return std::nullopt;

    std::optional<T> result;
    if (actualType == type && itemCount >= 1)
        result = static_cast<T>(*reinterpret_cast<long *>(data));

    XFree(data);
    return result;
}

QString applicationNameFromPid(unsigned long pid)
{
    QFile commFile(QStringLiteral("/proc/%1/comm").arg(pid));
    if (!commFile.open(QIODevice::ReadOnly))
        return QString();

    return QString::fromUtf8(commFile.readAll()).trimmed();
}

} // namespace

QString Utilities::focusedApplicationName()
{
    // No portable way for an unprivileged process to query the globally
    // focused window under Wayland; only X11 (including XWayland) is
    // handled below.
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return QString();

    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return QString();

    const Window root = DefaultRootWindow(display);
    const Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    const Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);

    QString name;
    if (activeWindowAtom != None && pidAtom != None) {
        if (const auto activeWindow = readProperty<Window>(display, root, activeWindowAtom, XA_WINDOW)) {
            if (const auto pid = readProperty<unsigned long>(display, *activeWindow, pidAtom, XA_CARDINAL))
                name = applicationNameFromPid(*pid);
        }
    }

    XCloseDisplay(display);
    return name;
}

#elif defined(Q_OS_WIN)

#    include <QFileInfo>

#    include <windows.h>

QString Utilities::focusedApplicationName()
{
    const HWND window = GetForegroundWindow();
    if (!window)
        return QString();

    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid == 0)
        return QString();

    const HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return QString();

    wchar_t path[MAX_PATH];
    DWORD size = MAX_PATH;
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &size);
    CloseHandle(process);

    if (!ok)
        return QString();

    return QFileInfo(QString::fromWCharArray(path, size)).completeBaseName();
}

#elif defined(Q_OS_MACOS)

// Implemented in utilities_mac.mm: querying the frontmost application
// requires Cocoa's NSWorkspace, which needs Objective-C++.

#else

QString Utilities::focusedApplicationName()
{
    return QString();
}

#endif
