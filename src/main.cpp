#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSystemTrayIcon>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.setInitialProperties({{"trayAvailable", QSystemTrayIcon::isSystemTrayAvailable()}});
    engine.loadFromModule("Worktime", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
