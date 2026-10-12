#pragma once

#include <QLoggingCategory>
#include <QObject>
#include <QPoint>
#include <QRect>

Q_DECLARE_LOGGING_CATEGORY(worktimeScreenGeometry)

/*!
 * \brief Exposes screen work-area geometry to QML, for placing the tray popup.
 *
 * QML's Screen type reports only the available size, not its origin, so the
 * work area (the screen minus taskbar, panel or menu bar) is read from C++.
 */
class ScreenGeometry : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    /*!
     * \brief Work area of the screen containing \a point.
     *
     * \return The work area, or the primary screen's work area if no screen
     * contains \a point.
     * \par Cyclomatic complexity: 2
     */
    Q_INVOKABLE QRect availableGeometryAt(const QPoint &point) const;

    /*!
     * \brief Work area of the primary screen, or an empty QRect if the
     * platform reports no primary screen at all (e.g. a misconfigured or
     * headless display server).
     * \par Cyclomatic complexity: 2
     */
    Q_INVOKABLE QRect primaryAvailableGeometry() const;
};
