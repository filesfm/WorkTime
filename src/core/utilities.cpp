#include "utilities.hpp"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QPixmap>
#include <QScreen>
#include <QSettings>
#include <QStandardPaths>

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

#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QJsonDocument>
#    include <QJsonObject>

QString Utilities::focusedApplicationName()
{
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
    }

    return title;
}

#elif defined(Q_OS_WIN)

#    include <windows.h>

QString Utilities::focusedApplicationName()
{
    HWND hwnd = GetForegroundWindow();
    if (!hwnd) {
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

#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusObjectPath>
#    include <QRandomGenerator>

void Utilities::autostart(bool autostart)
{
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
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);

    if (autostart)
        registry.setValue(QStringLiteral("WorkTime"), QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
    else
        registry.remove(QStringLiteral("WorkTime"));
}

#endif
