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

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)
#    include <QDBusArgument>
#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusObjectPath>
#    include <QDBusVirtualObject>
#    include <QVariantMap>
#endif

TEST(UtilitiesTest, FocusedApplicationNameDoesNotCrash)
{
    // No assumption on the returned value: whether a name is available
    // depends on the desktop session (X11 vs Wayland) and which window (if
    // any) currently has focus. This only guards against the call itself
    // crashing.
    (void)Utilities::focusedWindowTitle();
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

#if !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)

namespace {

class TestableUtilities : public Utilities
{
public:
    using Utilities::s_autostartPortalService;
};

/*
 * Stands in for xdg-desktop-portal: records every message it receives and
 * replies the way the real portal does, with a request object path.
 */
class FakeBackgroundPortal : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        calls.append(message);
        connection.send(message.createReply(
            QVariant::fromValue(QDBusObjectPath("/org/freedesktop/portal/desktop/request/1_0/worktime"))));
        return true;
    }
};

/*
 * Serves FakeBackgroundPortal on this process's own session-bus connection
 * and points autostart() at it, so tests never change the real autostart
 * configuration. Qt delivers calls addressed to a connection's own unique
 * name in-process, so autostart()'s blocking call reaches the fake directly.
 */
class UtilitiesAutostartTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/org/freedesktop/portal/desktop", &m_portal));
        m_previousService = TestableUtilities::s_autostartPortalService;
        TestableUtilities::s_autostartPortalService = m_bus.baseService();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableUtilities::s_autostartPortalService = m_previousService;
        m_bus.unregisterObject("/org/freedesktop/portal/desktop");
    }

    //! Options map (second argument) of the single call the fake portal received.
    QVariantMap onlyRequestOptions()
    {
        EXPECT_EQ(m_portal.calls.size(), 1);
        if (m_portal.calls.size() != 1 || m_portal.calls.constFirst().arguments().size() != 2)
            return {};
        return qdbus_cast<QVariantMap>(m_portal.calls.constFirst().arguments().at(1));
    }

    QDBusConnection m_bus{QString()};
    FakeBackgroundPortal m_portal;
    QString m_previousService;
};

} // namespace

TEST_F(UtilitiesAutostartTest, EnablingRequestsAutostartFromTheBackgroundPortal)
{
    Utilities::autostart(true);

    ASSERT_EQ(m_portal.calls.size(), 1);
    EXPECT_EQ(m_portal.calls.constFirst().interface(), QStringLiteral("org.freedesktop.portal.Background"));
    EXPECT_EQ(m_portal.calls.constFirst().member(), QStringLiteral("RequestBackground"));
    EXPECT_TRUE(onlyRequestOptions().value("autostart").toBool());
}

TEST_F(UtilitiesAutostartTest, DisablingRevokesAutostartFromTheBackgroundPortal)
{
    Utilities::autostart(false);

    const QVariantMap options = onlyRequestOptions();
    ASSERT_TRUE(options.contains("autostart"));
    EXPECT_FALSE(options.value("autostart").toBool());
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART) && defined(Q_OS_LINUX)