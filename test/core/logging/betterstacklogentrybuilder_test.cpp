#include <gtest/gtest.h>

#include <QByteArray>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#include "core/logging/betterstacklogentrybuilder.hpp"

namespace {

QJsonObject parse(const QByteArray &body)
{
    return QJsonDocument::fromJson(body).object();
}

// 2024-01-02 03:04:05.006 UTC, as milliseconds since epoch.
constexpr qint64 kTimestampMs = 1704164645006;

const QString kVersion = QStringLiteral("1.2.3");

} // namespace

TEST(BetterStackLogEntryBuilderTest, CriticalMessageHasCriticalLevel)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtCriticalMsg, QStringLiteral("disk full"), QString(), kTimestampMs, kVersion));

    EXPECT_EQ(entry["level"].toString(), QStringLiteral("critical"));
    EXPECT_EQ(entry["message"].toString(), QStringLiteral("disk full"));
}

TEST(BetterStackLogEntryBuilderTest, FatalMessageHasFatalLevel)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtFatalMsg, QStringLiteral("settings unwritable"), QString(), kTimestampMs, kVersion));

    EXPECT_EQ(entry["level"].toString(), QStringLiteral("fatal"));
}

TEST(BetterStackLogEntryBuilderTest, TimestampIsFormattedAsUtc)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtCriticalMsg, QStringLiteral("x"), QString(), kTimestampMs, kVersion));

    EXPECT_EQ(entry["dt"].toString(), QStringLiteral("2024-01-02 03:04:05.006"));
}

TEST(BetterStackLogEntryBuilderTest, NonEmptyCategoryIsIncludedAsSource)
{
    const QJsonObject entry = parse(buildBetterStackLogEntry(QtCriticalMsg,
                                                             QStringLiteral("x"),
                                                             QStringLiteral("worktime.settings"),
                                                             kTimestampMs,
                                                             kVersion));

    EXPECT_EQ(entry["source"].toString(), QStringLiteral("worktime.settings"));
}

TEST(BetterStackLogEntryBuilderTest, EmptyCategoryOmitsSource)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtCriticalMsg, QStringLiteral("x"), QString(), kTimestampMs, kVersion));

    EXPECT_FALSE(entry.contains("source"));
}

TEST(BetterStackLogEntryBuilderTest, DefaultCategoryOmitsSource)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtCriticalMsg, QStringLiteral("x"), QStringLiteral("default"), kTimestampMs, kVersion));

    EXPECT_FALSE(entry.contains("source"));
}

TEST(BetterStackLogEntryBuilderTest, AppVersionIsIncluded)
{
    const QJsonObject entry = parse(
        buildBetterStackLogEntry(QtFatalMsg, QStringLiteral("x"), QString(), kTimestampMs, kVersion));

    EXPECT_EQ(entry["app_version"].toString(), kVersion);
}

TEST(BetterStackLogEntryBuilderTest, MessageWithQuotesAndNewlinesIsValidJson)
{
    const QByteArray body = buildBetterStackLogEntry(QtCriticalMsg,
                                                     QStringLiteral("line one \"quoted\"\nline two"),
                                                     QString(),
                                                     kTimestampMs,
                                                     kVersion);

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    ASSERT_EQ(error.error, QJsonParseError::NoError);
    EXPECT_EQ(doc.object()["message"].toString(), QStringLiteral("line one \"quoted\"\nline two"));
}
