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
#if defined(BUILD_TESTING)
class OneInstanceGuarantor
#else
class OneInstanceGuarantor final
#endif
{
public:
    OneInstanceGuarantor();
#if defined(BUILD_TESTING)
    virtual ~OneInstanceGuarantor() = default;
#else
    ~OneInstanceGuarantor() = default;
#endif

    OneInstanceGuarantor(const OneInstanceGuarantor &) = delete;
    OneInstanceGuarantor &operator=(const OneInstanceGuarantor &) = delete;

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    static QString lockFilePath();

    QLockFile m_lockFile;
};
