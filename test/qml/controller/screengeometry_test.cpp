#include <gtest/gtest.h>

#include <QGuiApplication>
#include <QPoint>
#include <QRect>
#include <QScreen>

#include "qml/controller/screengeometry.hpp"

TEST(ScreenGeometryTest, PrimaryAvailableGeometryMatchesPrimaryScreen)
{
    ScreenGeometry geometry;
    const QScreen *primary = QGuiApplication::primaryScreen();
    ASSERT_NE(primary, nullptr);

    EXPECT_EQ(geometry.primaryAvailableGeometry(), primary->availableGeometry());
}

TEST(ScreenGeometryTest, AvailableGeometryAtPointInsidePrimaryScreenMatchesThatScreen)
{
    ScreenGeometry geometry;
    const QScreen *primary = QGuiApplication::primaryScreen();
    ASSERT_NE(primary, nullptr);

    EXPECT_EQ(geometry.availableGeometryAt(primary->geometry().center()), primary->availableGeometry());
}

TEST(ScreenGeometryTest, AvailableGeometryAtPointOutsideAnyScreenFallsBackToPrimary)
{
    ScreenGeometry geometry;
    const QScreen *primary = QGuiApplication::primaryScreen();
    ASSERT_NE(primary, nullptr);

    EXPECT_EQ(geometry.availableGeometryAt(QPoint(1000000, 1000000)), primary->availableGeometry());
}
