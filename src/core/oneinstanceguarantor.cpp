#include "oneinstanceguarantor.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QtSystemDetection>

#include <stdexcept>

#include <signal.h>

void OneInstanceGuarantor::createPidFile()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimeDir.isEmpty()) {
        throw std::runtime_error("Cannot find runtime dir.");
    }
    QString pidFilePath = QString("%1/files-fm-worktime.pid").arg(runtimeDir);

    QFileInfo pidFileInfo(pidFilePath);
    if (pidFileInfo.exists() && pidFileInfo.isFile()) {
        QFile pidFile(pidFilePath);
        if (!pidFile.open(QIODevice::ReadOnly)) {
            throw std::runtime_error("Cannot open pid file.");
        }
        QTextStream in(&pidFile);
        int pid;
        in >> pid;
        pidFile.close();

        if ((kill(pid, 0) == 0) || (errno == EPERM)) {
            throw std::runtime_error("One program instance already exists.");
        } else {
            if (pidFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QTextStream out(&pidFile);
                out << QCoreApplication::applicationPid();
                pidFile.close();
            } else {
                throw std::runtime_error("Cannot open pid file.");
            }
        }
    } else {
        QFile pidFile(pidFilePath);
        if (pidFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QTextStream out(&pidFile);
            out << QCoreApplication::applicationPid();
            pidFile.close();
        } else {
            throw std::runtime_error("Cannot create pid file.");
        }
    }
#elif defined(Q_OS_WINDOWS)
#endif
}

void OneInstanceGuarantor::deletePidFile()
{
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    QString runtimeDir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    QString pidFilePath = QString("%1/files-fm-worktime.pid").arg(runtimeDir);
    QFile::remove(pidFilePath);
#elif defined(Q_OS_WINDOWS)
#endif
}
