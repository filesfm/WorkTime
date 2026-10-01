#pragma once

#include <QLoggingCategory>
#include <QSqlDatabase>

#include <optional>

#include <cstddef>

Q_DECLARE_LOGGING_CATEGORY(worktimeSqliteConnection)

/*!
 * \brief Wraps a single SQLite connection.
 * \note Thread-safety: -
 *
 * The underlying connection is only usable from the thread that created it -
 * it must not be shared with, or its queries constructed from, any other
 * thread.
 */
#if defined(BUILD_TESTING)
class SQLiteConnection
#else
class SQLiteConnection final
#endif
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
     * Deletes all entries from the `sent` table whose `utc_timestamp` is
     * more than 24 hours older than the current time.
     *
     * \throws std::runtime_error if the operation fails.
     */
    void deleteEntriesOlderThan24HoursFromSentTable();

    /*!
     * \return The number of entries in the `sent` table whose
     * `utc_timestamp` is within the last 24 hours.
     * \throws std::runtime_error if the operation fails.
     */
    qint64 countEntriesInLast24HoursFromSentTable();

    /*!
     * \return The sum of `http_size` for entries in the `sent` table whose
     * `utc_timestamp` is within the last 24 hours.
     * \throws std::runtime_error if the operation fails.
     */
    qint64 sumHttpSizeInLast24HoursFromSentTable();

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

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    static QString filePath();

    void *operator new(std::size_t) = delete;
    void *operator new[](std::size_t) = delete;
    void *operator new(std::size_t, void *) = delete;
    void *operator new[](std::size_t, void *) = delete;

#if defined(BUILD_TESTING)
protected:
#else
private:
#endif
    QSqlDatabase m_databaseConnection;
};
