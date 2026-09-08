#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSystemTrayIcon>

#include "core/utilities.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Qt.labs.platform's SystemTrayIcon can't load an icon.source that
    // points into the Qt resource system (qrc:/...) on Linux - the DBus
    // StatusNotifierItem backend silently ends up with no icon at all, for
    // any image format. Extract it to a real file on disk once at startup
    // so the tray icon can reference it by a plain file:// path instead.
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {"trayAvailable", QSystemTrayIcon::isSystemTrayAvailable()},
        {"trayIconSource",
         Utilities::extractResourceToDisk(QStringLiteral(":/qt/qml/Worktime/icon.png"),
                                          QStringLiteral("worktime-tray-icon.png"))},
    });
    engine.loadFromModule("Worktime", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    QObject::connect(&engine, &QQmlApplicationEngine::quit, &QGuiApplication::quit);

    return app.exec();
}
