#pragma once

#include <QLockFile>
#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(worktimeOneInstanceGuarantor)

/*!
 * \brief RAII guard ensuring only one instance of the application runs at a time.
 *
 * Construction throws std::runtime_error if another instance is already
 * running, or if the lock file's directory can't be determined. Destruction
 * releases the lock.
 */
class OneInstanceGuarantor
{
public:
    OneInstanceGuarantor();

    OneInstanceGuarantor(const OneInstanceGuarantor &) = delete;
    OneInstanceGuarantor &operator=(const OneInstanceGuarantor &) = delete;

private:
    static QString lockFilePath();

    QLockFile m_lockFile;
};
