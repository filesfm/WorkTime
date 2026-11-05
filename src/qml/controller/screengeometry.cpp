#include "screengeometry.hpp"

#include <QGuiApplication>
#include <QScreen>

Q_LOGGING_CATEGORY(worktimeScreenGeometry, "worktime.screen.geometry")

QRect ScreenGeometry::availableGeometryAt(const QPoint &point) const
{
    if (const QScreen *screen = QGuiApplication::screenAt(point))
        return screen->availableGeometry();
    return primaryAvailableGeometry();
}

QRect ScreenGeometry::primaryAvailableGeometry() const
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        qCCritical(worktimeScreenGeometry) << "no primary screen reported by the platform";
        return QRect();
    }
    return screen->availableGeometry();
}
