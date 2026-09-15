#pragma once

#include <QHash>
#include <QObject>

class QSocketNotifier;

#if defined(Q_OS_LINUX)
struct udev;
struct udev_monitor;
#endif

/*!
 * \brief Emits activityDetected() on any global keyboard or mouse input,
 * even while no Worktime window has focus.
 */
class ActivityMonitor : public QObject
{
    Q_OBJECT

public:
    /*!
     * \brief Constructs the monitor without installing any hook yet (see start()).
     * \par Cyclomatic complexity: 1
     */
    explicit ActivityMonitor(QObject *parent = nullptr);
    /*!
     * \brief Calls stop() to release the platform hook, if still installed.
     * \par Cyclomatic complexity: 1
     */
    ~ActivityMonitor() override;

    /*!
     * \brief Installs the platform-specific global input hook. No-op if already running.
     */
    void start();
    /*!
     * \brief Removes the platform-specific global input hook. No-op if not running.
     */
    void stop();

signals:
    /*! \brief Emitted on any global key press/release or mouse click/move/scroll. */
    void activityDetected();

private:
#if defined(Q_OS_LINUX)
    void scanInputDevices();
    void addInputDevice(const QString &devNode);
    void removeInputDevice(const QString &devNode);
    void readInputDevice(int fd);
    void readUdevMonitor();

    udev *m_udev = nullptr;
    udev_monitor *m_udevMonitor = nullptr;
    QSocketNotifier *m_udevMonitorNotifier = nullptr;
    QHash<QString, QSocketNotifier *> m_deviceNotifiers;
#elif defined(Q_OS_WIN)
    void *m_keyboardHook = nullptr;
    void *m_mouseHook = nullptr;
#elif defined(Q_OS_MACOS)
    void *m_eventTap = nullptr;
    void *m_runLoopSource = nullptr;
#endif

    bool m_running = false;
};
