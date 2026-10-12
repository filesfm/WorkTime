#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

/*!
 * \brief Builds one Better Stack Logs HTTP-source event body for a single
 * critical/fatal Qt log message.
 *
 * \a type must be QtCriticalMsg or QtFatalMsg - it is encoded as the "fatal"
 * or "critical" \c level field, nothing else. \a category is the originating
 * QMessageLogContext::category (e.g. "worktime.settings"), or empty/"default"
 * for uncategorized messages, in which case the "source" field is omitted.
 * \a appVersion is the app's version string (e.g. "0.6.0"), always included
 * as the "app_version" field.
 *
 * Shared by BetterStackLogger (live messages) so the wire format stays in
 * one place and is unit-testable independently of installing a global Qt
 * message handler.
 *
 * \par Cyclomatic complexity: 2
 */
QByteArray buildBetterStackLogEntry(
    QtMsgType type, const QString &message, const QString &category, qint64 utcTimestampMs, const QString &appVersion);
