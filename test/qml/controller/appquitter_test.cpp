#include <gtest/gtest.h>

#include <QElapsedTimer>
#include <QEventLoop>
#include <QTimer>

#include "qml/controller/appquitter.hpp"

TEST(AppQuitterTest, QuitExitsTheRunningEventLoop)
{
    AppQuitter quitter;
    QEventLoop loop;

    // Safety net: if quit() stops working, fail fast instead of hanging CI.
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, &loop, &QEventLoop::quit);
    watchdog.start(3000);

    QElapsedTimer elapsed;
    elapsed.start();
    quitter.quit();
    loop.exec();

    EXPECT_LT(elapsed.elapsed(), 3000) << "event loop only exited via the watchdog, not quit()";
}
