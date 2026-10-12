#include "betterstacklogentrybuilder.hpp"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

QByteArray buildBetterStackLogEntry(QtMsgType type,
                                    const QString &message,
                                    const QString &category,
                                    qint64 utcTimestampMs)
{
    QJsonObject entry;
    entry[QStringLiteral("dt")] = QDateTime::fromMSecsSinceEpoch(utcTimestampMs, QTimeZone::UTC)
                                      .toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    entry[QStringLiteral("level")] = type == QtFatalMsg ? QStringLiteral("fatal") : QStringLiteral("critical");
    entry[QStringLiteral("message")] = message;
    if (!category.isEmpty() && category != QLatin1String("default"))
        entry[QStringLiteral("source")] = category;

    return QJsonDocument(entry).toJson(QJsonDocument::Compact);
}
