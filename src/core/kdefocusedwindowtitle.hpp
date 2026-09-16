#pragma once

#include <QObject>
#include <QString>
#include <QtClassHelperMacros>

/*!
 * \brief Tracks the focused window's title under KDE Plasma.
 */
class KDEFocusedWindowTitle : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(KDEFocusedWindowTitle)
    Q_CLASSINFO("D-Bus Interface", "io.github.filesfm.worktime.FocusedWindow")

public:
    /*!
     * \brief Returns the process-wide instance, creating it on first call.
     * \par Cyclomatic complexity: 1
     */
    static KDEFocusedWindowTitle *instance();

    /*!
     * \brief Registers the D-Bus service and (re)loads the KWin script that
     * feeds it. No-op if already started.
     * \par Cyclomatic complexity: 2
     */
    void start();

    /*!
     * \brief Unloads the KWin script and releases the D-Bus service. No-op
     * if not started.
     * \par Cyclomatic complexity: 2
     */
    void stop();

    /*!
     * \brief Title of the window that was last reported focused, or an
     * empty string if none has been reported yet (e.g. start() was never
     * called, or no window currently has focus).
     * \par Cyclomatic complexity: 1
     */
    QString activeWindowTitle() const;

public slots:
    /*!
     * \brief D-Bus-callable: records \a title as the focused window's title.
     * Called by the KWin script on every window activation.
     * \par Cyclomatic complexity: 1
     */
    Q_SCRIPTABLE void setActiveWindowTitle(const QString &title);

private:
    explicit KDEFocusedWindowTitle(QObject *parent = nullptr);

    QString m_activeWindowTitle;
    bool m_started = false;
};