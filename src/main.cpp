#include <QApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSystemTrayIcon>

#include <iostream>
#include <memory>

#include "core/oneinstanceguarantor.hpp"
#include "core/utilities.hpp"
#include "qml/controller/appquitter.hpp"
#include "qml/controller/popupplacement.hpp"
#include "qml/controller/screengeometry.hpp"

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

#if defined(Q_OS_LINUX)
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("gnome", Qt::CaseInsensitive)) {
        if (Utilities::isGNOMEFocusedWindowDBusInstalled()) {
            if (!Utilities::isGNOMEFocusedWindowDBusEnabled()) {
                Utilities::enableGNOMEFocusedWindowDBus();
            }
        } else {
            Utilities::installGNOMEFocusedWindowDBus();
            Utilities::enableGNOMEFocusedWindowDBus();
        }
    }
#endif

    QApplication app(argc, argv);

    app.setDesktopFileName(QStringLiteral("io.github.filesfm.worktime"));
    app.setQuitOnLastWindowClosed(false);

#if defined(Q_OS_LINUX)
    if (qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("kde", Qt::CaseInsensitive)) {
        KDEFocusedWindowTitle::instance()->start();
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { KDEFocusedWindowTitle::instance()->stop(); });
    }
#endif

    QQuickStyle::setStyle(QStringLiteral("Material"));

    QQmlApplicationEngine engine;
    AppQuitter appQuitter;
    engine.rootContext()->setContextProperty("appQuitter", &appQuitter);
    ScreenGeometry screenGeometry;
    engine.rootContext()->setContextProperty("screenGeometry", &screenGeometry);
    PopupPlacement popupPlacement;
    engine.rootContext()->setContextProperty("popupPlacement", &popupPlacement);
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    if (trayAvailable) {
        popupPlacement.start();
        QObject::connect(&app, &QCoreApplication::aboutToQuit, [&popupPlacement] { popupPlacement.stop(); });
    }
    engine.setInitialProperties({
        {"trayAvailable", trayAvailable},
#if defined(BUILD_WITHOUT_AUTOSTART)
        {"autoStartupAvailable", false},
#else
        {"autoStartupAvailable", true},
#endif
    });
    engine.loadFromModule("Worktime", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    QObject::connect(&engine, &QQmlApplicationEngine::quit, &appQuitter, &AppQuitter::quit);

    return app.exec();
}
