#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSystemTrayIcon>

#include <iostream>

#include <cstdlib>

#include "core/oneinstanceguarantor.hpp"
#include "core/utilities.hpp"
#include "qml/controller/appquitter.hpp"

#if defined(Q_OS_LINUX)
#    include "core/kdefocusedwindowtitle.hpp"
#endif

int main(int argc, char *argv[])
{
    QLoggingCategory::setFilterRules(QStringLiteral("worktime.*=false"));

    std::unique_ptr<OneInstanceGuarantor> instanceGuard;
    try {
        instanceGuard = std::make_unique<OneInstanceGuarantor>();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }

    QGuiApplication app(argc, argv);

    app.setDesktopFileName(QStringLiteral("io.github.filesfm.worktime"));
    app.setQuitOnLastWindowClosed(false);

#if defined(Q_OS_LINUX)
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP") == "KDE") {
        KDEFocusedWindowTitle::instance()->start();
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { KDEFocusedWindowTitle::instance()->stop(); });
    }
#endif

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
