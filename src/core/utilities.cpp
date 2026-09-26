#include "utilities.hpp"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>

#if defined(Q_OS_LINUX)
#    include <QDBusArgument>
#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusObjectPath>
#    include <QJsonArray>
#    include <QJsonDocument>
#    include <QJsonObject>
#    include <QRandomGenerator>

#    include "kdefocusedwindowtitle.hpp"
#elif defined(Q_OS_WINDOWS)
#    include <windows.h>
#endif

Q_LOGGING_CATEGORY(worktimeUtilities, "worktime.utilities")

QUrl Utilities::extractResourceToDisk(const QString &sourcePath, const QString &fileName)
{
    const QString destinationPath
        = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(fileName);
    QFile::remove(destinationPath);
    if (!QFile::copy(sourcePath, destinationPath))
        qCWarning(worktimeUtilities) << "failed to copy" << sourcePath << "to" << destinationPath;
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

QString Utilities::focusedWindowTitle()
{
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("gnome", Qt::CaseInsensitive)) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell",
                                                          "/org/gnome/shell/extensions/FocusedWindow",
                                                          "org.gnome.shell.extensions.FocusedWindow",
                                                          "Get");
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        QString title;

        if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
            const QString json = reply.arguments().at(0).toString();

            QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
            QJsonObject obj = doc.object();

            title = obj["title"].toString();
        } else {
            qCWarning(worktimeUtilities) << "GNOME FocusedWindow D-Bus call failed:" << reply.errorMessage();
        }

        return title;
    } else if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("kde", Qt::CaseInsensitive)) {
        return KDEFocusedWindowTitle::instance()->activeWindowTitle();
    } else if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("cinnamon", Qt::CaseInsensitive)) {
        QDBusMessage msg = QDBusMessage::createMethodCall("org.Cinnamon", "/org/Cinnamon", "org.Cinnamon", "Eval");
        msg << QVariant("global.display.focus_window.title");
        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        QString title;

        if (reply.type() == QDBusMessage::ReplyMessage && reply.arguments().size() >= 2) {
            const bool success = reply.arguments().at(0).toBool();
            const QString result = reply.arguments().at(1).toString();

            if (success) {
                QJsonDocument doc = QJsonDocument::fromJson('[' + result.toUtf8() + ']');
                if (doc.isArray() && !doc.array().isEmpty()) {
                    title = doc.array().first().toString();
                }
            }
        } else {
            qCWarning(worktimeUtilities) << "Cinnamon Eval D-Bus call failed:" << reply.errorMessage();
        }

        return title;
    }

    return QString();
}

#elif defined(Q_OS_WIN)

QString Utilities::focusedWindowTitle()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
        qCWarning(worktimeUtilities) << "GetForegroundWindow returned no window";
        return QString();
    }

    wchar_t title[256]{};
    GetWindowTextW(hwnd, title, std::size(title));

    return QString::fromWCharArray(title);
}

#elif defined(Q_OS_MACOS)

// Implemented in utilities_mac.mm

#endif

#if defined(Q_OS_LINUX)

void Utilities::autostart(bool autostart)
{
    qCInfo(worktimeUtilities) << (autostart ? "enabling" : "disabling") << "autostart";

    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop",
                                                      "/org/freedesktop/portal/desktop",
                                                      "org.freedesktop.portal.Background",
                                                      "RequestBackground");

    int token = QRandomGenerator::global()->bounded(1000, 9999);
    QMap<QString, QVariant> options = {{"autostart", autostart},
                                       {"background", autostart},
                                       {"reason", "Automatically launch application at login"},
                                       {"handle_token", QString("worktime_%1").arg(token)}};

    msg << "" << options;
    QDBusMessage response = bus.call(msg);

    if (response.type() != QDBusMessage::ReplyMessage) {
        qCWarning(worktimeUtilities) << "RequestBackground D-Bus call failed:" << response.errorMessage();
    }

    /*if (response.type() == QDBusMessage::ReplyMessage) {
        QDBusObjectPath handle = response.arguments().at(0).value<QDBusObjectPath>();
        bus.connect("org.freedesktop.portal.Desktop",
                    handle.path(),
                    "org.freedesktop.portal.Request",
                    "Response",
                    this,
                    SLOT(handleFlatpakResponse(uint, QVariantMap)));
    }*/
}

#elif defined(Q_OS_MACOS)

#elif defined(Q_OS_WINDOWS)

void Utilities::autostart(bool autostart)
{
    qCInfo(worktimeUtilities) << (autostart ? "enabling" : "disabling") << "autostart";

    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);

    if (autostart)
        registry.setValue(QStringLiteral("WorkTime"), QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    else
        registry.remove(QStringLiteral("WorkTime"));
}

#endif

#if defined(Q_OS_LINUX)

bool Utilities::isGNOMEFocusedWindowDBusInstalled()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell.Extensions",
                                                      "/org/gnome/Shell/Extensions",
                                                      "org.gnome.Shell.Extensions",
                                                      "GetExtensionInfo");

    msg << QStringLiteral("focused-window-dbus@flexagoon.com");
    QDBusMessage response = bus.call(msg);

    if (response.type() != QDBusMessage::ReplyMessage || response.arguments().isEmpty()) {
        qCWarning(worktimeUtilities) << "GetExtensionInfo D-Bus call failed:" << response.errorMessage();
        return false;
    }

    return !qdbus_cast<QVariantMap>(response.arguments().constFirst()).isEmpty();
}

bool Utilities::isGNOMEFocusedWindowDBusEnabled()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell.Extensions",
                                                      "/org/gnome/Shell/Extensions",
                                                      "org.gnome.Shell.Extensions",
                                                      "GetExtensionInfo");

    msg << QStringLiteral("focused-window-dbus@flexagoon.com");
    QDBusMessage response = bus.call(msg);

    if (response.type() != QDBusMessage::ReplyMessage || response.arguments().isEmpty()) {
        qCWarning(worktimeUtilities) << "GetExtensionInfo D-Bus call failed:" << response.errorMessage();
        return false;
    }

    const QVariantMap info = qdbus_cast<QVariantMap>(response.arguments().constFirst());
    return info.value("state").toUInt() == 1;
}

void Utilities::installGNOMEFocusedWindowDBus()
{
    qCInfo(worktimeUtilities) << "installing GNOME focused-window D-Bus extension";

    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell.Extensions",
                                                      "/org/gnome/Shell/Extensions",
                                                      "org.gnome.Shell.Extensions",
                                                      "InstallRemoteExtension");

    msg << QStringLiteral("focused-window-dbus@flexagoon.com");
    QDBusMessage response = bus.call(msg);

    if (response.type() != QDBusMessage::ReplyMessage) {
        qCWarning(worktimeUtilities) << "InstallRemoteExtension D-Bus call failed:" << response.errorMessage();
    }
}

void Utilities::enableGNOMEFocusedWindowDBus()
{
    qCInfo(worktimeUtilities) << "enabling GNOME focused-window D-Bus extension";

    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage msg = QDBusMessage::createMethodCall("org.gnome.Shell.Extensions",
                                                      "/org/gnome/Shell/Extensions",
                                                      "org.gnome.Shell.Extensions",
                                                      "EnableExtension");

    msg << QStringLiteral("focused-window-dbus@flexagoon.com");
    QDBusMessage response = bus.call(msg);

    if (response.type() != QDBusMessage::ReplyMessage) {
        qCWarning(worktimeUtilities) << "EnableExtension D-Bus call failed:" << response.errorMessage();
    }
}

#endif

#if defined(Q_OS_LINUX)

void Utilities::showNotification(const QString &title, const QString &message)
{
    QDBusMessage msg = QDBusMessage::createMethodCall("org.freedesktop.Notifications",
                                                      "/org/freedesktop/Notifications",
                                                      "org.freedesktop.Notifications",
                                                      "Notify");

    msg << QStringLiteral("WorkTime") << quint32(0) << QStringLiteral("io.github.filesfm.worktime") << title << message
        << QStringList() << QVariantMap() << qint32(-1);

    QDBusMessage response = QDBusConnection::sessionBus().call(msg);

    if (response.type() != QDBusMessage::ReplyMessage)
        qCWarning(worktimeUtilities) << "Notify D-Bus call failed:" << response.errorMessage();
}

#elif defined(Q_OS_WIN)

void Utilities::showNotification(const QString &title, const QString &message)
{
    qCWarning(worktimeUtilities) << "showNotification not yet implemented on Windows:" << title << message;
}

#elif defined(Q_OS_MACOS)

// Implemented in utilities_mac.mm

#endif
