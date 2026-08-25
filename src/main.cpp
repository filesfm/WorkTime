#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QUrl>

namespace {

// Qt.labs.platform's SystemTrayIcon can't load an icon.source that points
// into the Qt resource system (qrc:/...) on Linux - the DBus
// StatusNotifierItem backend silently ends up with no icon at all, for any
// image format. Extract it to a real file on disk once at startup so the
// tray icon can reference it by a plain file:// path instead.
QUrl extractTrayIconToDisk()
{
    const QString path = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(QStringLiteral("worktime-tray-icon.png"));
    QFile::remove(path);
    QFile::copy(QStringLiteral(":/qt/qml/Worktime/icon.png"), path);
    return QUrl::fromLocalFile(path);
}

} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"trayAvailable", QSystemTrayIcon::isSystemTrayAvailable()},
        {"trayIconSource", extractTrayIconToDisk()},
    });
    engine.loadFromModule("Worktime", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
