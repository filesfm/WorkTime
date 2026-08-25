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

TEST(UtilitiesTest, CaptureScreenshotsReturnsOneImagePerScreen)
{
    const QList<QScreen *> screens = QGuiApplication::screens();
    const QList<QImage> images = Utilities::captureScreenshots();

    ASSERT_EQ(images.size(), screens.size());
    for (qsizetype i = 0; i < images.size(); ++i) {
        EXPECT_FALSE(images.at(i).isNull());
        EXPECT_EQ(images.at(i).size(), screens.at(i)->geometry().size());
    }
}

TEST(UtilitiesTest, FocusedApplicationNameDoesNotCrash)
{
    // No assumption on the returned value: whether a name is available
    // depends on the desktop session (X11 vs Wayland) and which window (if
    // any) currently has focus. This only guards against the call itself
    // crashing.
    (void)Utilities::focusedApplicationName();
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

#endif
