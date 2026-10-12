#pragma once

#include <QLoggingCategory>
#include <QString>
#include <QtGlobal>

class QMessageLogContext;

Q_DECLARE_LOGGING_CATEGORY(worktimeBetterStackLogger)

/*!
 * \brief Forwards Qt's critical and fatal log messages to Better Stack Logs.
 *
 * The destination (ingesting host + source token) is compiled in from the
 * BETTERSTACK_SOURCE_TOKEN/BETTERSTACK_INGESTING_HOST CMake cache variables
 * (see betterstackconfig.hpp.in) - there is no runtime setting and nothing
 * for a user to configure. If either is unset at build time, install() is a
 * no-op and the app behaves exactly as it did before this class existed.
 *
 * Never instantiated: every member is static.
 */
class BetterStackLogger
{
public:
    BetterStackLogger() = delete;

    /*!
     * \brief Installs the process-wide Qt message handler, unless no
     * destination was compiled in. Call once, after QCoreApplication exists
     * (the handler performs synchronous network I/O that needs it).
     * \par Cyclomatic complexity: 2
     */
    static void install();

private:
    /*!
     * \brief Prints \a message exactly as Qt's default handler would, but
     * only if QT_LOGGING_RULES is set in the environment - otherwise nothing
     * is printed to stdout/stderr, regardless of message type. Independently
     * of that, QtCriticalMsg/QtFatalMsg are always forwarded to Better Stack
     * via send(); debug/info/warning messages are never forwarded.
     * \par Cyclomatic complexity: 3
     */
    static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message);

    /*!
     * \brief Synchronously POSTs one log entry to the compiled-in Better
     * Stack ingesting host, with a short timeout, so a QtFatalMsg's entry
     * has a chance to be delivered before Qt aborts the process once this
     * call returns. Failures are reported via qCWarning(), never qCCritical/
     * qCFatal, to avoid recursing back into this same handler.
     * \par Cyclomatic complexity: 3
     */
    static void send(QtMsgType type, const QString &message, const QString &category);
};
