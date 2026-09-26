#include "sqliteconnection.hpp"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <atomic>
#include <stdexcept>

Q_LOGGING_CATEGORY(worktimeSqliteConnection, "worktime.sqlite.connection")

namespace {

std::atomic<quint64> nextConnectionId{0};

QString nextConnectionName()
{
    return QStringLiteral("SQLiteConnection_%1").arg(nextConnectionId.fetch_add(1));
}

} // namespace

SQLiteConnection::SQLiteConnection()
    : m_databaseConnection{QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nextConnectionName())}
{
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataDir);

    const QString databaseFilePath = QDir(dataDir).filePath(QString::fromUtf8(databasePath()));
    qCDebug(worktimeSqliteConnection) << "opening" << m_databaseConnection.connectionName() << "at" << databaseFilePath;

    m_databaseConnection.setDatabaseName(databaseFilePath);
    m_databaseConnection.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));

    if (!m_databaseConnection.open()) {
        qCCritical(worktimeSqliteConnection) << "failed to open database:" << m_databaseConnection.lastError().text();
        throw std::runtime_error("Cannot open the database: " + m_databaseConnection.lastError().text().toStdString());
    }

    QSqlQuery query(m_databaseConnection);
    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS sent ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "focused_window_title TEXT, "
                                   "utc_timestamp INTEGER NOT NULL, "
                                   "shoot_time INTEGER NOT NULL"
                                   ")"))) {
        qCCritical(worktimeSqliteConnection) << "failed to create sent table:" << query.lastError().text();
        throw std::runtime_error("Cannot create the sent table: " + query.lastError().text().toStdString());
    }

    if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS not_sent ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "focused_window_title TEXT, "
                                   "utc_timestamp INTEGER NOT NULL, "
                                   "shoot_time INTEGER NOT NULL, "
                                   "http_size INTEGER NOT NULL"
                                   ")"))) {
        qCCritical(worktimeSqliteConnection) << "failed to create not_sent table:" << query.lastError().text();
        throw std::runtime_error("Cannot create the not_sent table: " + query.lastError().text().toStdString());
    }

    qCDebug(worktimeSqliteConnection) << m_databaseConnection.connectionName() << "ready";
}

SQLiteConnection::~SQLiteConnection()
{
    const QString connectionName = m_databaseConnection.connectionName();
    qCDebug(worktimeSqliteConnection) << "closing" << connectionName;

    m_databaseConnection.close();
    QSqlDatabase::removeDatabase(connectionName);
}

void SQLiteConnection::addEntryToSentTable(const QString &focusedWindowTitle, qint64 utcTimestamp, qint64 shootTime)
{
    QSqlQuery query(m_databaseConnection);
    query.prepare(QStringLiteral("INSERT INTO sent (focused_window_title, utc_timestamp, shoot_time) "
                                 "VALUES (:focused_window_title, :utc_timestamp, :shoot_time)"));
    query.bindValue(":focused_window_title", focusedWindowTitle);
    query.bindValue(":utc_timestamp", utcTimestamp);
    query.bindValue(":shoot_time", shootTime);

    if (!query.exec()) {
        qCWarning(worktimeSqliteConnection) << "failed to insert sent entry:" << query.lastError().text();
        throw std::runtime_error("Cannot insert sent entry: " + query.lastError().text().toStdString());
    }

    qCDebug(worktimeSqliteConnection) << "inserted sent entry" << query.lastInsertId();
}

std::optional<SQLiteConnection::SentTableRow> SQLiteConnection::takeOldestEntryFromSentTable()
{
    if (!m_databaseConnection.transaction()) {
        qCWarning(worktimeSqliteConnection)
            << "failed to start transaction:" << m_databaseConnection.lastError().text();
        throw std::runtime_error("Cannot start transaction: " + m_databaseConnection.lastError().text().toStdString());
    }

    QSqlQuery selectQuery(m_databaseConnection);
    selectQuery.prepare(
        QStringLiteral("SELECT id, focused_window_title, utc_timestamp, shoot_time FROM sent ORDER BY id ASC LIMIT 1"));

    if (!selectQuery.exec()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection) << "failed to select oldest sent entry:" << selectQuery.lastError().text();
        throw std::runtime_error("Cannot select oldest sent entry: " + selectQuery.lastError().text().toStdString());
    }

    if (!selectQuery.next()) {
        m_databaseConnection.rollback();
        qCDebug(worktimeSqliteConnection) << "sent table is empty";
        return std::nullopt;
    }

    const int id = selectQuery.value(0).toInt();
    SentTableRow row;
    row.focusedWindowTitle = selectQuery.value(1).toString();
    row.utcTimestamp = selectQuery.value(2).toLongLong();
    row.shootTime = selectQuery.value(3).toLongLong();

    QSqlQuery deleteQuery(m_databaseConnection);
    deleteQuery.prepare(QStringLiteral("DELETE FROM sent WHERE id = :id"));
    deleteQuery.bindValue(":id", id);

    if (!deleteQuery.exec()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection) << "failed to delete sent entry:" << deleteQuery.lastError().text();
        throw std::runtime_error("Cannot delete sent entry: " + deleteQuery.lastError().text().toStdString());
    }

    if (!m_databaseConnection.commit()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection)
            << "failed to commit transaction:" << m_databaseConnection.lastError().text();
        throw std::runtime_error("Cannot commit transaction: " + m_databaseConnection.lastError().text().toStdString());
    }

    qCDebug(worktimeSqliteConnection) << "took sent entry" << id;

    return row;
}

void SQLiteConnection::addEntryToNotSentTable(const QString &focusedWindowTitle,
                                              qint64 utcTimestamp,
                                              qint64 shootTime,
                                              qint64 httpSize)
{
    QSqlQuery query(m_databaseConnection);
    query.prepare(QStringLiteral("INSERT INTO not_sent (focused_window_title, utc_timestamp, shoot_time, http_size) "
                                 "VALUES (:focused_window_title, :utc_timestamp, :shoot_time, :http_size)"));
    query.bindValue(":focused_window_title", focusedWindowTitle);
    query.bindValue(":utc_timestamp", utcTimestamp);
    query.bindValue(":shoot_time", shootTime);
    query.bindValue(":http_size", httpSize);

    if (!query.exec()) {
        qCWarning(worktimeSqliteConnection) << "failed to insert not_sent entry:" << query.lastError().text();
        throw std::runtime_error("Cannot insert not_sent entry: " + query.lastError().text().toStdString());
    }

    qCDebug(worktimeSqliteConnection) << "inserted not_sent entry" << query.lastInsertId();
}

std::optional<SQLiteConnection::NotSentTableRow> SQLiteConnection::takeOldestEntryFromNotSentTable()
{
    if (!m_databaseConnection.transaction()) {
        qCWarning(worktimeSqliteConnection)
            << "failed to start transaction:" << m_databaseConnection.lastError().text();
        throw std::runtime_error("Cannot start transaction: " + m_databaseConnection.lastError().text().toStdString());
    }

    QSqlQuery selectQuery(m_databaseConnection);
    selectQuery.prepare(QStringLiteral("SELECT id, focused_window_title, utc_timestamp, shoot_time, http_size "
                                       "FROM not_sent ORDER BY id ASC LIMIT 1"));

    if (!selectQuery.exec()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection)
            << "failed to select oldest not_sent entry:" << selectQuery.lastError().text();
        throw std::runtime_error("Cannot select oldest not_sent entry: " + selectQuery.lastError().text().toStdString());
    }

    if (!selectQuery.next()) {
        m_databaseConnection.rollback();
        qCDebug(worktimeSqliteConnection) << "not_sent table is empty";
        return std::nullopt;
    }

    const int id = selectQuery.value(0).toInt();
    NotSentTableRow row;
    row.focusedWindowTitle = selectQuery.value(1).toString();
    row.utcTimestamp = selectQuery.value(2).toLongLong();
    row.shootTime = selectQuery.value(3).toLongLong();
    row.httpSize = selectQuery.value(4).toLongLong();

    QSqlQuery deleteQuery(m_databaseConnection);
    deleteQuery.prepare(QStringLiteral("DELETE FROM not_sent WHERE id = :id"));
    deleteQuery.bindValue(":id", id);

    if (!deleteQuery.exec()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection) << "failed to delete not_sent entry:" << deleteQuery.lastError().text();
        throw std::runtime_error("Cannot delete not_sent entry: " + deleteQuery.lastError().text().toStdString());
    }

    if (!m_databaseConnection.commit()) {
        m_databaseConnection.rollback();
        qCWarning(worktimeSqliteConnection)
            << "failed to commit transaction:" << m_databaseConnection.lastError().text();
        throw std::runtime_error("Cannot commit transaction: " + m_databaseConnection.lastError().text().toStdString());
    }

    qCDebug(worktimeSqliteConnection) << "took not_sent entry" << id;

    return row;
}
