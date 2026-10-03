#include "screengeometry.hpp"

#include <QGuiApplication>
#include <QScreen>

QRect ScreenGeometry::availableGeometryAt(const QPoint &point) const
{
    if (QScreen *screen = QGuiApplication::screenAt(point))
        return screen->availableGeometry();
    return primaryAvailableGeometry();
}

QRect ScreenGeometry::primaryAvailableGeometry() const
{
    return QGuiApplication::primaryScreen()->availableGeometry();
}
