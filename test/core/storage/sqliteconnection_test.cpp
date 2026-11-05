#include <gtest/gtest.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QString>

#include "core/storage/sqliteconnection.hpp"

namespace {

class TestableSQLiteConnection : public SQLiteConnection
{
public:
    using SQLiteConnection::filePath;
    using SQLiteConnection::m_databaseConnection;
};

QString databaseFilePath()
{
    return TestableSQLiteConnection::filePath();
}

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

TEST_F(SQLiteConnectionTest, CountEntriesInLast24HoursFromSentTableCountsOnlyRecentEntries)
{
    SQLiteConnection connection;

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 oldTimestamp = now - 25 * 60 * 60;
    const qint64 recentTimestamp = now - 60;

    connection.addEntryToSentTable("old-window", oldTimestamp, oldTimestamp, 100);
    connection.addEntryToSentTable("recent-window1", recentTimestamp, recentTimestamp, 200);
    connection.addEntryToSentTable("recent-window2", recentTimestamp, recentTimestamp, 300);

    EXPECT_EQ(connection.countEntriesInLast24HoursFromSentTable(), 2);
}

TEST_F(SQLiteConnectionTest, CountEntriesInLast24HoursFromEmptySentTableReturnsZero)
{
    SQLiteConnection connection;

    EXPECT_EQ(connection.countEntriesInLast24HoursFromSentTable(), 0);
}

TEST_F(SQLiteConnectionTest, SumHttpSizeInLast24HoursFromSentTableSumsOnlyRecentEntries)
{
    SQLiteConnection connection;

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 oldTimestamp = now - 25 * 60 * 60;
    const qint64 recentTimestamp = now - 60;

    connection.addEntryToSentTable("old-window", oldTimestamp, oldTimestamp, 1000);
    connection.addEntryToSentTable("recent-window1", recentTimestamp, recentTimestamp, 200);
    connection.addEntryToSentTable("recent-window2", recentTimestamp, recentTimestamp, 300);

    EXPECT_EQ(connection.sumHttpSizeInLast24HoursFromSentTable(), 500);
}

TEST_F(SQLiteConnectionTest, SumHttpSizeInLast24HoursFromEmptySentTableReturnsZero)
{
    SQLiteConnection connection;

    EXPECT_EQ(connection.sumHttpSizeInLast24HoursFromSentTable(), 0);
}

TEST_F(SQLiteConnectionTest, DeleteEntriesOlderThan24HoursFromSentTableRemovesOnlyOldEntries)
{
    SQLiteConnection connection;

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 oldTimestamp = now - 25 * 60 * 60;
    const qint64 recentTimestamp = now - 60;

    connection.addEntryToSentTable("old-window", oldTimestamp, oldTimestamp, 100);
    connection.addEntryToSentTable("recent-window", recentTimestamp, recentTimestamp, 200);

    connection.deleteEntriesOlderThan24HoursFromSentTable();

    const auto row = connection.takeOldestEntryFromSentTable();
    ASSERT_TRUE(row.has_value());
    EXPECT_EQ(row->focusedWindowTitle, QStringLiteral("recent-window"));
    EXPECT_EQ(connection.takeOldestEntryFromSentTable(), std::nullopt);
}

TEST_F(SQLiteConnectionTest, DeleteEntriesOlderThan24HoursFromSentTableLeavesNotSentTableAlone)
{
    SQLiteConnection connection;

    const qint64 oldTimestamp = QDateTime::currentSecsSinceEpoch() - 25 * 60 * 60;
    connection.addEntryToNotSentTable("old-not-sent", oldTimestamp, oldTimestamp, 100);

    connection.deleteEntriesOlderThan24HoursFromSentTable();

    EXPECT_TRUE(connection.takeOldestEntryFromNotSentTable().has_value());
}

TEST_F(SQLiteConnectionTest, DeleteEntriesOlderThan24HoursFromEmptySentTableDoesNotThrow)
{
    SQLiteConnection connection;

    EXPECT_NO_THROW(connection.deleteEntriesOlderThan24HoursFromSentTable());
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

TEST_F(SQLiteConnectionTest, FilePathMethodReturnsAbsoluteFilePath)
{
    const QString path{TestableSQLiteConnection::filePath()};
    EXPECT_TRUE(QDir::isAbsolutePath(path));
}

TEST_F(SQLiteConnectionTest, ConstructorSetsCorrectDatabaseName)
{
    TestableSQLiteConnection connection;
    EXPECT_EQ(connection.m_databaseConnection.databaseName(), TestableSQLiteConnection::filePath());
}

TEST_F(SQLiteConnectionTest, ConstructorThrowsWhenDatabaseCannotBeOpened)
{
    QFile file{databaseFilePath()};
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.close();
    const QFileDevice::Permissions startPermissions{file.permissions()};
    ASSERT_TRUE(file.setPermissions(QFileDevice::Permissions()));

    EXPECT_THROW(SQLiteConnection connection, std::runtime_error);

    file.setPermissions(startPermissions);
}

TEST_F(SQLiteConnectionTest, AddEntryToSentTableThrowsWhenConnectionIsClosed)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.addEntryToSentTable("window", 1, 2, 3), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, AddEntryToNotSentTableThrowsWhenConnectionIsClosed)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.addEntryToNotSentTable("window", 1, 2, 3), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, DeleteEntriesOlderThan24HoursThrowsWhenConnectionIsClosed)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.deleteEntriesOlderThan24HoursFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, CountEntriesInLast24HoursThrowsWhenConnectionIsClosed)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.countEntriesInLast24HoursFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, SumHttpSizeInLast24HoursThrowsWhenConnectionIsClosed)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.sumHttpSizeInLast24HoursFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromSentTableThrowsWhenTransactionCannotStart)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.takeOldestEntryFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromNotSentTableThrowsWhenTransactionCannotStart)
{
    TestableSQLiteConnection connection;
    connection.m_databaseConnection.close();

    EXPECT_THROW(connection.takeOldestEntryFromNotSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromSentTableThrowsWhenSelectFails)
{
    TestableSQLiteConnection connection;
    QSqlQuery dropQuery(connection.m_databaseConnection);
    ASSERT_TRUE(dropQuery.exec(QStringLiteral("DROP TABLE sent")));

    EXPECT_THROW(connection.takeOldestEntryFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromNotSentTableThrowsWhenSelectFails)
{
    TestableSQLiteConnection connection;
    QSqlQuery dropQuery(connection.m_databaseConnection);
    ASSERT_TRUE(dropQuery.exec(QStringLiteral("DROP TABLE not_sent")));

    EXPECT_THROW(connection.takeOldestEntryFromNotSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromSentTableThrowsWhenDeleteFails)
{
    TestableSQLiteConnection connection;
    connection.addEntryToSentTable("window", 1, 2, 3);

    // query_only lets the transaction start and the SELECT succeed, but
    // rejects the DELETE - isolating that one failure branch.
    QSqlQuery pragmaQuery(connection.m_databaseConnection);
    ASSERT_TRUE(pragmaQuery.exec(QStringLiteral("PRAGMA query_only = ON")));

    EXPECT_THROW(connection.takeOldestEntryFromSentTable(), std::runtime_error);
}

TEST_F(SQLiteConnectionTest, TakeOldestFromNotSentTableThrowsWhenDeleteFails)
{
    TestableSQLiteConnection connection;
    connection.addEntryToNotSentTable("window", 1, 2, 3);

    QSqlQuery pragmaQuery(connection.m_databaseConnection);
    ASSERT_TRUE(pragmaQuery.exec(QStringLiteral("PRAGMA query_only = ON")));

    EXPECT_THROW(connection.takeOldestEntryFromNotSentTable(), std::runtime_error);
}