#include "oneinstanceguarantor.hpp"

#include <QStandardPaths>
#include <QtSystemDetection>

#include <stdexcept>

Q_LOGGING_CATEGORY(worktimeOneInstanceGuarantor, "worktime.one.instance.guarantor")

QString OneInstanceGuarantor::lockFilePath()
{
    QString runtimeDir;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
#elif defined(Q_OS_WINDOWS)
    runtimeDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
#endif
    if (runtimeDir.isEmpty()) {
        qCCritical(worktimeOneInstanceGuarantor) << "cannot find runtime dir";
        throw std::runtime_error("Cannot find runtime dir.");
    }

    return QString("%1/files-fm-worktime.lock").arg(runtimeDir);
}

OneInstanceGuarantor::OneInstanceGuarantor()
{
    qCInfo(worktimeOneInstanceGuarantor) << "acquiring single-instance lock at" << m_lockFile.fileName();

    m_lockFile.setStaleLockTime(0);

    if (!m_lockFile.tryLock()) {
        qint64 pid = 0;
        QString hostname;
        m_lockFile.getLockInfo(&pid, &hostname, nullptr);
        qCCritical(worktimeOneInstanceGuarantor)
            << "another instance is already running (pid" << pid << "on" << hostname << ")";
        throw std::runtime_error("One program instance already exists.");
    }

    qCInfo(worktimeOneInstanceGuarantor) << "lock acquired";
}
