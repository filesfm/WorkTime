#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>

#include "core/utilities.hpp"
#include "qml/controller/appquitter.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setQuitOnLastWindowClosed(false);

    QQmlApplicationEngine engine;
    AppQuitter appQuitter;
    engine.rootContext()->setContextProperty("appQuitter", &appQuitter);
    engine.setInitialProperties({
        {"trayAvailable", QSystemTrayIcon::isSystemTrayAvailable()},
        {"trayIconSource",
         Utilities::extractResourceToDisk(QStringLiteral(":/qt/qml/Worktime/icon.png"),
                                          QStringLiteral("worktime-tray-icon.png"))},
    });
    engine.loadFromModule("Worktime", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    QObject::connect(&engine, &QQmlApplicationEngine::quit, &appQuitter, &AppQuitter::quit);

    return app.exec();
}
