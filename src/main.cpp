#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>

#include <iostream>

#include <cstdlib>

#include "core/oneinstanceguarantor.h"
#include "core/utilities.hpp"
#include "qml/controller/appquitter.hpp"

int main(int argc, char *argv[])
{
    try {
        OneInstanceGuarantor::createPidFile();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::atexit(&OneInstanceGuarantor::deletePidFile);

    QGuiApplication app(argc, argv);

    app.setDesktopFileName(QStringLiteral("io.github.filesfm.worktime"));
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
