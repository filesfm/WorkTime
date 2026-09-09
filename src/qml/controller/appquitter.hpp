#pragma once

#include <QObject>

/*!
 * \brief Exposes an app-quit action to QML, for the tray menu's Quit item.
 */
class AppQuitter : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    /*!
     * \brief Quits the application, safely from any QML call stack.
     * \par Cyclomatic complexity: 1
     */
    Q_INVOKABLE void quit();
};
