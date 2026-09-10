#include "utilities.hpp"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>

QList<QImage> Utilities::captureScreenshots()
{
    QList<QImage> images;
    for (QScreen *screen : QGuiApplication::screens())
        images.append(screen->grabWindow(0).toImage());

    return images;
}

QUrl Utilities::extractResourceToDisk(const QString &sourcePath, const QString &fileName)
{
    const QString destinationPath
        = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(fileName);
    QFile::remove(destinationPath);
    QFile::copy(sourcePath, destinationPath);
    return QUrl::fromLocalFile(destinationPath);
}

QString Utilities::truncateUtf8Safe(const QString &value, qsizetype maxCodePoints)
{
    const QList<uint> codePoints = value.toUcs4();
    if (codePoints.size() <= maxCodePoints)
        return value;

    return QString::fromUcs4(reinterpret_cast<const char32_t *>(codePoints.constData()), maxCodePoints);
}

#if defined(Q_OS_LINUX)

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

// Finds the currently active window (`_NET_ACTIVE_WINDOW` on the root
// window), shared by focusedApplicationName(), activeWindowTitle(), and
// activeWindowExecutablePath() so each doesn't repeat the same lookup.
std::optional<Window> activeWindow(Display *display)
{
    const Atom activeWindowAtom = XInternAtom(display, "_NET_ACTIVE_WINDOW", True);
    if (activeWindowAtom == None)
        return std::nullopt;

    return readProperty<Window>(display, DefaultRootWindow(display), activeWindowAtom, XA_WINDOW);
}

// PID of the process owning `window` (`_NET_WM_PID`), shared by
// focusedApplicationName() and activeWindowExecutablePath().
std::optional<unsigned long> windowPid(Display *display, Window window)
{
    const Atom pidAtom = XInternAtom(display, "_NET_WM_PID", True);
    if (pidAtom == None)
        return std::nullopt;

    return readProperty<unsigned long>(display, window, pidAtom, XA_CARDINAL);
}

// Title of `window`: `_NET_WM_NAME` (UTF8_STRING) if set, else the legacy
// `WM_NAME` (Latin-1, via XFetchName).
QString windowTitle(Display *display, Window window)
{
    const Atom utf8StringAtom = XInternAtom(display, "UTF8_STRING", True);
    const Atom netWmNameAtom = XInternAtom(display, "_NET_WM_NAME", True);

    if (utf8StringAtom != None && netWmNameAtom != None) {
        Atom actualType;
        int actualFormat = 0;
        unsigned long itemCount = 0;
        unsigned long bytesAfter = 0;
        unsigned char *data = nullptr;

        const int status = XGetWindowProperty(display,
                                              window,
                                              netWmNameAtom,
                                              0,
                                              // Property length is measured in 32-bit units; this
                                              // covers titles up to ~4000 bytes, comfortably above
                                              // the 255-character limit this value is truncated to.
                                              1024,
                                              False,
                                              utf8StringAtom,
                                              &actualType,
                                              &actualFormat,
                                              &itemCount,
                                              &bytesAfter,
                                              &data);

        QString title;
        if (status == Success && data) {
            if (actualType == utf8StringAtom && itemCount > 0)
                title = QString::fromUtf8(reinterpret_cast<const char *>(data), static_cast<int>(itemCount));
            XFree(data);
        }
        if (!title.isEmpty())
            return title;
    }

    char *legacyName = nullptr;
    if (XFetchName(display, window, &legacyName) != 0 && legacyName) {
        const QString title = QString::fromLatin1(legacyName);
        XFree(legacyName);
        return title;
    }

    return QString();
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

    QString name;
    if (const auto window = activeWindow(display)) {
        if (const auto pid = windowPid(display, *window))
            name = applicationNameFromPid(*pid);
    }

    XCloseDisplay(display);
    return name;
}

QString Utilities::activeWindowTitle()
{
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return QString();

    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return QString();

    QString title;
    if (const auto window = activeWindow(display))
        title = windowTitle(display, *window);

    XCloseDisplay(display);
    return title;
}

QString Utilities::activeWindowExecutablePath()
{
    if (qEnvironmentVariableIsSet("WAYLAND_DISPLAY"))
        return QString();

    Display *display = XOpenDisplay(nullptr);
    if (!display)
        return QString();

    QString path;
    if (const auto window = activeWindow(display)) {
        if (const auto pid = windowPid(display, *window))
            path = QFile::symLinkTarget(QStringLiteral("/proc/%1/exe").arg(*pid));
    }

    XCloseDisplay(display);
    return path;
}

#elif defined(Q_OS_WIN)

#    include <QFileInfo>
#    include <QVarLengthArray>

#    include <windows.h>

namespace {

// Full path to the executable owning the current foreground window, or
// empty if it could not be determined. Shared by focusedApplicationName()
// and activeWindowExecutablePath() so each doesn't repeat the same lookup.
QString foregroundWindowExecutablePath()
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

    return QString::fromWCharArray(path, size);
}

} // namespace

QString Utilities::focusedApplicationName()
{
    const QString path = foregroundWindowExecutablePath();
    return path.isEmpty() ? QString() : QFileInfo(path).completeBaseName();
}

QString Utilities::activeWindowTitle()
{
    const HWND window = GetForegroundWindow();
    if (!window)
        return QString();

    const int length = GetWindowTextLengthW(window);
    if (length <= 0)
        return QString();

    // +1 for the terminating null GetWindowTextW always writes.
    QVarLengthArray<wchar_t, 256> buffer(length + 1);
    const int copied = GetWindowTextW(window, buffer.data(), buffer.size());
    if (copied <= 0)
        return QString();

    return QString::fromWCharArray(buffer.data(), copied);
}

QString Utilities::activeWindowExecutablePath()
{
    return foregroundWindowExecutablePath();
}

#elif defined(Q_OS_MACOS)

// Implemented in utilities_mac.mm: querying the frontmost application and
// its focused window requires Cocoa's NSWorkspace and the Accessibility
// API, which need Objective-C++.

#else

QString Utilities::focusedApplicationName()
{
    return QString();
}

QString Utilities::activeWindowTitle()
{
    return QString();
}

QString Utilities::activeWindowExecutablePath()
{
    return QString();
}

#endif

#if defined(Q_OS_LINUX)

void Utilities::autostart(bool autostart) {}

#elif defined(Q_OS_MACOS)

void Utilities::autostart(bool autostart) {}

#elif defined(Q_OS_WINDOWS)

void Utilities::autostart(bool autostart)
{
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);

    if (autostart)
        registry.setValue(QStringLiteral("WorkTime"), QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    else
        registry.remove(QStringLiteral("WorkTime"));
}

#endif
