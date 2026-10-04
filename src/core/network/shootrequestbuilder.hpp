#pragma once

#include <QByteArray>
#include <QString>

/*!
 * \brief Builds the form-urlencoded request body for one activity sample,
 * using the Worktime remote endpoint's `Shoot[...]` field names.
 *
 * Shared by SendingService (live samples) and NotSentSender (resent queued
 * samples) so both post an identical wire format.
 *
 * \par Cyclomatic complexity: 1
 */
QByteArray buildShootRequestBody(const QString &title, qint64 utcTimestamp, qint64 shootTime);
