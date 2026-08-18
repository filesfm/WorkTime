#pragma once

#include <QImage>
#include <QList>
#include <QString>

/*!
 * \brief Stateless OS-interaction helpers: screenshot capture and focused
 * application detection.
 */
class Utilities
{
public:
    /*!
     * \brief Deleted: Utilities is never instantiated, only used via its static members.
     * \par Cyclomatic complexity: N/A (deleted - no body)
     */
    Utilities() = delete;

    /*!
     * \brief Captures every connected screen.
     * \return One image per screen, in QGuiApplication::screens() order.
     * Empty if no screens are available.
     * \par Cyclomatic complexity: 2
     */
    static QList<QImage> captureScreenshots();

    /*!
     * \brief Name of the application owning the currently focused window.
     *
     * \return The focused application's name (its process's base
     * executable name, without path or extension), or an empty string if it
     * could not be determined.
     * \par Cyclomatic complexity: 7 (Linux), 5 (Windows), 2 (macOS), 1 (other platforms) - the
     * implementation differs per platform (see utilities.cpp / utilities_mac.mm).
     */
    static QString focusedApplicationName();
};
