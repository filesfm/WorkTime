#pragma once

#include <QJSValue>
#include <QObject>

/*!
 * \brief Opens the tray menu on macOS once the status item has finished tracking the mouse click.
 */
class TrayMenuOpener : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    /*!
     * \brief Calls \a openMenu when the main run loop is back in its default mode.
     * \par Cyclomatic complexity: 1
     */
    Q_INVOKABLE void open(const QJSValue &openMenu);
};
