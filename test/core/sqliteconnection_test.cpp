#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>

#include "core/sqliteconnection.hpp"

namespace {

// Exposes the protected databasePath() so the test can locate the database
// file on disk without hardcoding its name separately from the class.
class TestableSQLiteConnection : public SQLiteConnection
{
public:
    using SQLiteConnection::databasePath;
};

QString databaseFilePath()
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);
    return QDir(dataDir).filePath(QString::fromUtf8(TestableSQLiteConnection::databasePath()));
}

// SQLiteConnection always opens the same fixed path
// (QStandardPaths::AppLocalDataLocation + databasePath()), so every instance
// in the process shares one file on disk. Removing it before each test keeps
// tests independent despite that shared, fixed location.
class SQLiteConnectionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(databaseFilePath());
    }
};

} // namespace

TEST_F(SQLiteConnectionTest, ConstructionCreatesEmptyTables)
{
    SQLiteConnection connection;

    EXPECT_EQ(connection.takeOldestEntryFromSentTable(), std::nullopt);
    EXPECT_EQ(connection.takeOldestEntryFromNotSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, AddAndTakeOldestFromSentTable)
{
    SQLiteConnection connection;
    connection.addEntryToSentTable("window1", 1000, 2000, 100);

    const auto row = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QStringLiteral("window1"));
    EXPECT_EQ(row->utcTimestamp, 1000);
    EXPECT_EQ(row->shootTime, 2000);
    EXPECT_EQ(row->httpSize, 100);
}

TEST_F(SQLiteConnectionTest, AddAndTakeOldestFromNotSentTable)
{
    SQLiteConnection connection;
    connection.addEntryToNotSentTable("window2", 3000, 4000, 200);

    const auto row = connection.takeOldestEntryFromNotSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QStringLiteral("window2"));
    EXPECT_EQ(row->utcTimestamp, 3000);
    EXPECT_EQ(row->shootTime, 4000);
    EXPECT_EQ(row->httpSize, 200);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromEmptySentTableReturnsNullopt)
{
    SQLiteConnection connection;
    EXPECT_EQ(connection.takeOldestEntryFromSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromEmptyNotSentTableReturnsNullopt)
{
    SQLiteConnection connection;
    EXPECT_EQ(connection.takeOldestEntryFromNotSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, TakeOldestRemovesEntryFromSentTable)
{
    SQLiteConnection connection;
    connection.addEntryToSentTable("window1", 1000, 2000, 100);

    ASSERT_TRUE(connection.takeOldestEntryFromSentTable().has_value());
    EXPECT_EQ(connection.takeOldestEntryFromSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, TakeOldestRemovesEntryFromNotSentTable)
{
    SQLiteConnection connection;
    connection.addEntryToNotSentTable("window1", 1000, 2000, 100);

    ASSERT_TRUE(connection.takeOldestEntryFromNotSentTable().has_value());
    EXPECT_EQ(connection.takeOldestEntryFromNotSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, SentTableIsFirstInFirstOut)
{
    SQLiteConnection connection;
    connection.addEntryToSentTable("first", 1, 1, 1);
    connection.addEntryToSentTable("second", 2, 2, 2);
    connection.addEntryToSentTable("third", 3, 3, 3);

    const auto first = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->focusedWindowTitle, QStringLiteral("first"));

    const auto second = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->focusedWindowTitle, QStringLiteral("second"));

    const auto third = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(third->focusedWindowTitle, QStringLiteral("third"));

    EXPECT_EQ(connection.takeOldestEntryFromSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, NotSentTableIsFirstInFirstOut)
{
    SQLiteConnection connection;
    connection.addEntryToNotSentTable("first", 1, 1, 1);
    connection.addEntryToNotSentTable("second", 2, 2, 2);

    const auto first = connection.takeOldestEntryFromNotSentTable();
    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first->focusedWindowTitle, QStringLiteral("first"));

    const auto second = connection.takeOldestEntryFromNotSentTable();
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(second->focusedWindowTitle, QStringLiteral("second"));
}

TEST_F(SQLiteConnectionTest, SentAndNotSentTablesAreIndependent)
{
    SQLiteConnection connection;
    connection.addEntryToSentTable("sent-entry", 1, 1, 1);

    EXPECT_EQ(connection.takeOldestEntryFromNotSentTable(), std::nullopt);

    const auto row = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QStringLiteral("sent-entry"));
}

TEST_F(SQLiteConnectionTest, EntriesPersistAcrossConnectionInstances)
{
    {
        SQLiteConnection connection;
        connection.addEntryToSentTable("persisted", 42, 43, 44);
    }

    SQLiteConnection reopened;
    const auto row = reopened.takeOldestEntryFromSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QStringLiteral("persisted"));
    EXPECT_EQ(row->utcTimestamp, 42);
    EXPECT_EQ(row->shootTime, 43);
    EXPECT_EQ(row->httpSize, 44);
}

TEST_F(SQLiteConnectionTest, EmptyFocusedWindowTitleIsPreserved)
{
    SQLiteConnection connection;
    connection.addEntryToSentTable("", 1, 2, 3);

    const auto row = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QString());
}
