#include <gtest/gtest.h>

#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QScreen>
#include <QString>

#include "core/utilities.hpp"

TEST(UtilitiesTest, CaptureScreenshotsReturnsOneImagePerScreen)
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    const QList<QImage> images = Utilities::captureScreenshots();

    ASSERT_EQ(images.size(), screens.size());
    for (qsizetype i = 0; i < images.size(); ++i) {
        EXPECT_FALSE(images.at(i).isNull());
        EXPECT_EQ(images.at(i).size(), screens.at(i)->geometry().size());
    }
}

TEST(UtilitiesTest, FocusedApplicationNameDoesNotCrash)
{
    // No assumption on the returned value: whether a name is available
    // depends on the desktop session (X11 vs Wayland) and which window (if
    // any) currently has focus. This only guards against the call itself
    // crashing.
    (void)Utilities::focusedApplicationName();
}

#if defined(Q_OS_LINUX)

TEST(UtilitiesTest, FocusedApplicationNameIsEmptyUnderWayland)
{
    const QByteArray previousValue = qgetenv("WAYLAND_DISPLAY");
    const bool hadWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");

    qputenv("WAYLAND_DISPLAY", "wayland-0");
    EXPECT_TRUE(Utilities::focusedApplicationName().isEmpty());

    if (hadWaylandDisplay)
        qputenv("WAYLAND_DISPLAY", previousValue);
    else
        qunsetenv("WAYLAND_DISPLAY");
}

#endif
