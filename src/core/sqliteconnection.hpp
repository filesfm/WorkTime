#pragma once

#include <QLoggingCategory>
#include <QSqlDatabase>

#include <optional>

#include <cstddef>

Q_DECLARE_LOGGING_CATEGORY(worktimeSqliteConnection)

/*!
 * \brief Wraps a single SQLite connection.
 *
 * The underlying connection is only usable from the thread that created it -
 * it must not be shared with, or its queries constructed from, any other
 * thread.
 */
class SQLiteConnection final
{
public:
    struct SentTableRow
    {
        QString focusedWindowTitle;
        qint64 utcTimestamp = 0;
        qint64 shootTime = 0;
        qint64 httpSize = 0;
    };
    struct NotSentTableRow
    {
        QString focusedWindowTitle;
        qint64 utcTimestamp = 0;
        qint64 shootTime = 0;
        qint64 httpSize = 0;
    };

    /*!
     * \throws std::runtime_error if the database can't be opened, or the
     * `sent`/`not_sent` tables can't be created.
     */
    SQLiteConnection();
    ~SQLiteConnection();

    /*!
     * \throws std::runtime_error if the entry can't be inserted.
     */
    void addEntryToSentTable(const QString &focusedWindowTitle, qint64 utcTimestamp, qint64 shootTime, qint64 httpSize);

    /*!
     * \return The oldest entry, or std::nullopt if the table is empty.
     * \throws std::runtime_error if the operation fails.
     */
    std::optional<SentTableRow> takeOldestEntryFromSentTable();

    /*!
     * \throws std::runtime_error if the entry can't be inserted.
     */
    void addEntryToNotSentTable(const QString &focusedWindowTitle,
                                qint64 utcTimestamp,
                                qint64 shootTime,
                                qint64 httpSize);

    /*!
     * \return The oldest entry, or std::nullopt if the table is empty.
     * \throws std::runtime_error if the operation fails.
     */
    std::optional<NotSentTableRow> takeOldestEntryFromNotSentTable();

private:
    static constexpr const char *databasePath() { return "worktime.sqlite"; }

    void *operator new(std::size_t) = delete;
    void *operator new[](std::size_t) = delete;
    void *operator new(std::size_t, void *) = delete;
    void *operator new[](std::size_t, void *) = delete;

private:
    QSqlDatabase m_databaseConnection;
};
