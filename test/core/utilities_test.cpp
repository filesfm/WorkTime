#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QList>
#include <QScreen>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QUrl>

#include "core/utilities.hpp"

TEST(UtilitiesTest, FocusedApplicationNameDoesNotCrash)
{
    // No assumption on the returned value: whether a name is available
    // depends on the desktop session (X11 vs Wayland) and which window (if
    // any) currently has focus. This only guards against the call itself
    // crashing.
    (void)Utilities::focusedApplicationName();
}

TEST(UtilitiesTest, ActiveWindowTitleDoesNotCrash)
{
    // Same caveat as FocusedApplicationNameDoesNotCrash: no assumption on
    // the returned value, only that the call itself doesn't crash.
    (void)Utilities::activeWindowTitle();
}

TEST(UtilitiesTest, ActiveWindowExecutablePathDoesNotCrash)
{
    // Same caveat as FocusedApplicationNameDoesNotCrash: no assumption on
    // the returned value, only that the call itself doesn't crash.
    (void)Utilities::activeWindowExecutablePath();
}

TEST(UtilitiesTest, ExtractResourceToDiskCopiesContentAndReturnsFileUrl)
{
    QTemporaryDir sourceDir;
    ASSERT_TRUE(sourceDir.isValid());
    const QString sourcePath = sourceDir.filePath("source.txt");
    QFile sourceFile(sourcePath);
    ASSERT_TRUE(sourceFile.open(QIODevice::WriteOnly));
    sourceFile.write("hello");
    sourceFile.close();

    const QUrl url = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-copy.txt");

    ASSERT_TRUE(url.isLocalFile());
    EXPECT_EQ(QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                  .filePath("worktime-utilities-test-copy.txt"),
              url.toLocalFile());

    QFile destinationFile(url.toLocalFile());
    ASSERT_TRUE(destinationFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(destinationFile.readAll(), QByteArray("hello"));
    destinationFile.close();

    QFile::remove(url.toLocalFile());
}

TEST(UtilitiesTest, ExtractResourceToDiskOverwritesAnExistingDestination)
{
    QTemporaryDir sourceDir;
    ASSERT_TRUE(sourceDir.isValid());
    const QString sourcePath = sourceDir.filePath("source.txt");

    QFile firstSource(sourcePath);
    ASSERT_TRUE(firstSource.open(QIODevice::WriteOnly));
    firstSource.write("first");
    firstSource.close();
    const QUrl firstUrl = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-overwrite.txt");

    QFile secondSource(sourcePath);
    ASSERT_TRUE(secondSource.open(QIODevice::WriteOnly | QIODevice::Truncate));
    secondSource.write("second");
    secondSource.close();
    const QUrl secondUrl = Utilities::extractResourceToDisk(sourcePath, "worktime-utilities-test-overwrite.txt");

    QFile destinationFile(secondUrl.toLocalFile());
    ASSERT_TRUE(destinationFile.open(QIODevice::ReadOnly));
    EXPECT_EQ(destinationFile.readAll(), QByteArray("second"));
    destinationFile.close();

    QFile::remove(firstUrl.toLocalFile());
}

TEST(UtilitiesTest, TruncateUtf8SafeValueUnderLimitIsUnchanged)
{
    EXPECT_EQ(Utilities::truncateUtf8Safe("hello", 255), QStringLiteral("hello"));
}

TEST(UtilitiesTest, TruncateUtf8SafeValueAtLimitIsUnchanged)
{
    const QString value(255, QLatin1Char('a'));
    EXPECT_EQ(Utilities::truncateUtf8Safe(value, 255), value);
}

TEST(UtilitiesTest, TruncateUtf8SafeValueOverLimitIsTruncatedToExactLength)
{
    const QString value(300, QLatin1Char('a'));
    const QString truncated = Utilities::truncateUtf8Safe(value, 255);

    EXPECT_EQ(truncated.length(), 255);
    EXPECT_EQ(truncated, QString(255, QLatin1Char('a')));
}

// The 255th code point is an astral character (U+1F600, outside the Basic
// Multilingual Plane) represented in UTF-16 as a surrogate pair. Truncating
// at a UTF-16 code-unit boundary instead of a code-point boundary would slice
// the pair in half, producing an unpaired surrogate that can't be encoded to
// valid UTF-8.
TEST(UtilitiesTest, TruncateUtf8SafeDoesNotSplitAnAstralCharacterAtTheBoundary)
{
    QString value(254, QLatin1Char('a'));
    value.append(QChar::highSurrogate(0x1F600));
    value.append(QChar::lowSurrogate(0x1F600));
    value.append(QLatin1Char('z'));

    const QString truncated = Utilities::truncateUtf8Safe(value, 255);

    ASSERT_EQ(truncated.toUcs4().size(), 255);
    EXPECT_TRUE(truncated.toUtf8().size() > 0);
    EXPECT_FALSE(truncated.endsWith(QChar::highSurrogate(0x1F600)));
}

TEST(UtilitiesTest, TruncateUtf8SafeEmptyValueIsUnchanged)
{
    EXPECT_EQ(Utilities::truncateUtf8Safe(QString(), 255), QString());
}

#if defined(Q_OS_LINUX)

TEST(UtilitiesTest, FocusedApplicationNameIsEmptyUnderWayland)
{
    const QByteArray previousValue = qgetenv("WAYLAND_DISPLAY");
    const bool hadWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");

    qputenv("WAYLAND_DISPLAY", "wayland-0");
    EXPECT_TRUE(Utilities::focusedApplicationName().isEmpty());

    if (hadWaylandDisplay)
        qputenv("WAYLAND_DISPLAY", previousValue);
    else
        qunsetenv("WAYLAND_DISPLAY");
}

TEST(UtilitiesTest, ActiveWindowTitleIsEmptyUnderWayland)
{
    const QByteArray previousValue = qgetenv("WAYLAND_DISPLAY");
    const bool hadWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");

    qputenv("WAYLAND_DISPLAY", "wayland-0");
    EXPECT_TRUE(Utilities::activeWindowTitle().isEmpty());

    if (hadWaylandDisplay)
        qputenv("WAYLAND_DISPLAY", previousValue);
    else
        qunsetenv("WAYLAND_DISPLAY");
}

TEST(UtilitiesTest, ActiveWindowExecutablePathIsEmptyUnderWayland)
{
    const QByteArray previousValue = qgetenv("WAYLAND_DISPLAY");
    const bool hadWaylandDisplay = qEnvironmentVariableIsSet("WAYLAND_DISPLAY");

    qputenv("WAYLAND_DISPLAY", "wayland-0");
    EXPECT_TRUE(Utilities::activeWindowExecutablePath().isEmpty());

    if (hadWaylandDisplay)
        qputenv("WAYLAND_DISPLAY", previousValue);
    else
        qunsetenv("WAYLAND_DISPLAY");
}

#endif
