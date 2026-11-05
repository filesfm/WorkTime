#include <gtest/gtest.h>

#include <QLoggingCategory>
#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QVariant>

#include "core/settings/settings.hpp"

#if defined(Q_OS_LINUX)
#    include <QDBusArgument>
#    include <QDBusConnection>
#    include <QDBusError>
#    include <QDBusMessage>
#    include <QDBusObjectPath>
#    include <QDBusVariant>
#    include <QDBusVirtualObject>
#    include <QList>

#    include "core/settings/credentialstore.hpp"
#endif

namespace {

class SettingsEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
        QLoggingCategory::setFilterRules(QStringLiteral("worktime.settings.debug=true"));

        QStandardPaths::setTestModeEnabled(true);
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.clear();
        raw.sync();
    }
};

::testing::Environment *const g_settingsEnv = ::testing::AddGlobalTestEnvironment(new SettingsEnvironment);

} // namespace

TEST(SettingsTest, DefaultsArePopulatedOnFirstUse)
{
    Settings *settings = Settings::instance();

    EXPECT_EQ(settings->username(), QString());
    EXPECT_EQ(settings->password(), QString());
    EXPECT_FALSE(settings->autoStartup());
    EXPECT_FALSE(settings->startButtonPushed());
}

TEST(SettingsTest, UsernameRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setUsername("alice");
    EXPECT_EQ(settings->username(), QStringLiteral("alice"));
    settings->setUsername("bob");
    EXPECT_EQ(settings->username(), QStringLiteral("bob"));
    settings->setUsername("alan");
    EXPECT_EQ(settings->username(), QStringLiteral("alan"));
}

TEST(SettingsTest, PasswordRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setPassword("hunter2");
    EXPECT_EQ(settings->password(), QStringLiteral("hunter2"));
    settings->setPassword("thebestpasswordInTHeWoRLD");
    EXPECT_EQ(settings->password(), QStringLiteral("thebestpasswordInTHeWoRLD"));
    settings->setPassword("2");
    EXPECT_EQ(settings->password(), QStringLiteral("2"));
}

TEST(SettingsTest, AutoStartupRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setAutoStartup(true);
    EXPECT_TRUE(settings->autoStartup());
    settings->setAutoStartup(false);
    EXPECT_FALSE(settings->autoStartup());
}

TEST(SettingsTest, StartButtonPushedRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setStartButtonPushed(true);
    EXPECT_TRUE(settings->startButtonPushed());
    settings->setStartButtonPushed(false);
    EXPECT_FALSE(settings->startButtonPushed());
}

TEST(SettingsTest, SetUsernameEmitsSignalOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setUsername("initial");

    QSignalSpy spy(settings, &Settings::usernameChanged);

    settings->setUsername("initial");
    EXPECT_EQ(spy.count(), 0);

    settings->setUsername("changed");
    EXPECT_EQ(spy.count(), 1);
}

TEST(SettingsTest, SetPasswordEmitsSignalOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setPassword("initial");

    QSignalSpy spy(settings, &Settings::passwordChanged);

    settings->setPassword("initial");
    EXPECT_EQ(spy.count(), 0);

    settings->setPassword("changed");
    EXPECT_EQ(spy.count(), 1);
}

TEST(SettingsTest, SetAutoStartupEmitsSignalOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setAutoStartup(true);

    QSignalSpy spy(settings, &Settings::autoStartupChanged);

    settings->setAutoStartup(true);
    EXPECT_EQ(spy.count(), 0);

    settings->setAutoStartup(false);
    EXPECT_EQ(spy.count(), 1);
}

TEST(SettingsTest, SetStartButtonPushedEmitsSignalOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setStartButtonPushed(true);

    QSignalSpy spy(settings, &Settings::startButtonChanged);

    settings->setStartButtonPushed(true);
    EXPECT_EQ(spy.count(), 0);

    settings->setStartButtonPushed(false);
    EXPECT_EQ(spy.count(), 1);
}

namespace {

class TestableSettings : public Settings
{};

} // namespace

TEST(SettingsTest, SanitizeFillsMissingKeyWithDefault)
{
    {
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.remove("username");
        raw.sync();
    }

    TestableSettings settings;

    EXPECT_EQ(settings.username(), QString());
}

TEST(SettingsTest, SanitizeRemovesUnknownKeys)
{
    {
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.setValue("unknownOption", "leftover");
        raw.sync();
    }

    TestableSettings settings;

    QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
    EXPECT_FALSE(raw.contains("unknownOption"));
}

TEST(SettingsTest, SanitizeResetsInvalidValueToDefault)
{
    {
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.setValue("autoStartup", QVariant(QVariantList{1, 2, 3}));
        raw.sync();
    }

    TestableSettings settings;

    EXPECT_FALSE(settings.autoStartup());
}

TEST(SettingsTest, ConstructorLogsSettingsFilePath)
{
    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    TestableSettings settings;

    qInstallMessageHandler(previous);

    EXPECT_TRUE(captured.contains("loading settings from"));
}

TEST(SettingsTest, SetUsernameLogsDebugAlwaysAndInfoOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setUsername("initial");

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    settings->setUsername("initial");
    EXPECT_TRUE(captured.contains("setUsername called with"));
    EXPECT_FALSE(captured.contains("username changed to"));

    captured.clear();
    settings->setUsername("changed");
    EXPECT_TRUE(captured.contains("setUsername called with"));
    EXPECT_TRUE(captured.contains("username changed to"));

    qInstallMessageHandler(previous);
}

TEST(SettingsTest, SetPasswordLogsDebugAlwaysAndInfoOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setPassword("initial");

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    settings->setPassword("initial");
    EXPECT_TRUE(captured.contains("setPassword called"));
    EXPECT_FALSE(captured.contains("password changed"));

    captured.clear();
    settings->setPassword("changed");
    EXPECT_TRUE(captured.contains("setPassword called"));
    EXPECT_TRUE(captured.contains("password changed"));

    qInstallMessageHandler(previous);
}

TEST(SettingsTest, SetAutoStartupLogsDebugAlwaysAndInfoOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setAutoStartup(true);

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    settings->setAutoStartup(true);
    EXPECT_TRUE(captured.contains("setAutoStartup called with"));
    EXPECT_FALSE(captured.contains("autoStartup changed to"));

    captured.clear();
    settings->setAutoStartup(false);
    EXPECT_TRUE(captured.contains("setAutoStartup called with"));
    EXPECT_TRUE(captured.contains("autoStartup changed to"));

    qInstallMessageHandler(previous);
}

TEST(SettingsTest, SetStartButtonPushedLogsDebugAlwaysAndInfoOnlyWhenChanged)
{
    Settings *settings = Settings::instance();
    settings->setStartButtonPushed(true);

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    settings->setStartButtonPushed(true);
    EXPECT_TRUE(captured.contains("setStartButtonPushed called with"));
    EXPECT_FALSE(captured.contains("startButtonPushed changed to"));

    captured.clear();
    settings->setStartButtonPushed(false);
    EXPECT_TRUE(captured.contains("setStartButtonPushed called with"));
    EXPECT_TRUE(captured.contains("startButtonPushed changed to"));

    qInstallMessageHandler(previous);
}

TEST(SettingsTest, SanitizeLogsUnknownOptionRemoval)
{
    {
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.setValue("unknownOption", "leftover");
        raw.sync();
    }

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    TestableSettings settings;

    EXPECT_TRUE(captured.contains("deleting unknown option"));
    EXPECT_TRUE(captured.contains("unknownOption"));

    qInstallMessageHandler(previous);
}

TEST(SettingsTest, SanitizeLogsResetToDefaultForMissingKey)
{
    {
        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.remove("username");
        raw.sync();
    }

    static QString captured;
    captured.clear();

    QtMessageHandler previous = qInstallMessageHandler(
        [](QtMsgType, const QMessageLogContext &context, const QString &msg) {
            if (qstrcmp(context.category, "worktime.settings") == 0)
                captured += msg + "\n";
        });

    TestableSettings settings;

    EXPECT_TRUE(captured.contains("resetting"));
    EXPECT_TRUE(captured.contains("username"));
    EXPECT_TRUE(captured.contains("to its default value"));

    qInstallMessageHandler(previous);
}

#if defined(Q_OS_LINUX)

namespace {

class TestableCredentialStore : public CredentialStore
{
public:
    using CredentialStore::s_defaultCollectionPath;
    using CredentialStore::s_secretServiceName;
    using CredentialStore::s_secretServicePath;
};

// Minimal Secret Service fake: SearchItems always reports nothing stored
// (so readPassword() returns empty, exercising loadPassword()'s migration
// path rather than its "already cached" early-out), and CreateItem's
// success is controllable, so writePassword() - the migration itself - can
// be made to succeed or fail on demand.
class FakeSecretService : public QDBusVirtualObject
{
public:
    bool reportExistingItem = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        if (message.member() == QStringLiteral("OpenSession")) {
            connection.send(message.createReply(
                QList<QVariant>{QVariant::fromValue(QDBusVariant(QString())),
                                QVariant::fromValue(QDBusObjectPath(QStringLiteral("/session/s1")))}));
        } else if (message.member() == QStringLiteral("SearchItems")) {
            QList<QDBusObjectPath> unlocked;
            if (reportExistingItem)
                unlocked.append(QDBusObjectPath(QStringLiteral("/secrets/item")));
            connection.send(message.createReply(
                QList<QVariant>{QVariant::fromValue(unlocked), QVariant::fromValue(QList<QDBusObjectPath>{})}));
        }
        return true;
    }
};

// Stands in for a stored item, for the "already has a cached password"
// migration branch, which needs readPassword() to return something.
class FakeItem : public QDBusVirtualObject
{
public:
    QByteArray secretValue = "existing-credential-store-password";

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        if (message.member() == QStringLiteral("GetSecret")) {
            QDBusArgument secretArg;
            secretArg.beginStructure();
            secretArg << QDBusObjectPath(QStringLiteral("/session/s1")) << QByteArray() << secretValue
                      << QStringLiteral("text/plain; charset=utf8");
            secretArg.endStructure();
            connection.send(message.createReply(QVariant::fromValue(secretArg)));
        }
        return true;
    }
};

class FakeCollection : public QDBusVirtualObject
{
public:
    bool createItemShouldFail = false;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;

        if (message.member() == QStringLiteral("CreateItem")) {
            if (createItemShouldFail) {
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("cannot create")));
            } else {
                connection.send(message.createReply(
                    QList<QVariant>{QVariant::fromValue(QDBusObjectPath(QStringLiteral("/item/i1"))),
                                    QVariant::fromValue(QDBusObjectPath(QStringLiteral("/")))}));
            }
        }
        return true;
    }
};

class SettingsPasswordMigrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject("/secrets", &m_service));
        ASSERT_TRUE(m_bus.registerVirtualObject("/secrets/collection", &m_collection));
        ASSERT_TRUE(m_bus.registerVirtualObject("/secrets/item", &m_item));
        m_previousServiceName = TestableCredentialStore::s_secretServiceName;
        m_previousServicePath = TestableCredentialStore::s_secretServicePath;
        m_previousCollectionPath = TestableCredentialStore::s_defaultCollectionPath;
        TestableCredentialStore::s_secretServiceName = m_bus.baseService();
        TestableCredentialStore::s_secretServicePath = QStringLiteral("/secrets");
        TestableCredentialStore::s_defaultCollectionPath = QStringLiteral("/secrets/collection");

        QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
        raw.setValue("password", "legacy-plaintext-password");
        raw.sync();
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableCredentialStore::s_secretServiceName = m_previousServiceName;
        TestableCredentialStore::s_secretServicePath = m_previousServicePath;
        TestableCredentialStore::s_defaultCollectionPath = m_previousCollectionPath;
        m_bus.unregisterObject("/secrets");
        m_bus.unregisterObject("/secrets/collection");
        m_bus.unregisterObject("/secrets/item");
    }

    QDBusConnection m_bus{QString()};
    FakeSecretService m_service;
    FakeCollection m_collection;
    FakeItem m_item;
    QString m_previousServiceName;
    QString m_previousServicePath;
    QString m_previousCollectionPath;
};

} // namespace

TEST_F(SettingsPasswordMigrationTest, SuccessfulMigrationCachesPasswordAndRemovesLegacyEntry)
{
    TestableSettings settings;

    EXPECT_EQ(settings.password(), QStringLiteral("legacy-plaintext-password"));

    QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
    EXPECT_FALSE(raw.contains("password"));
}

TEST_F(SettingsPasswordMigrationTest, FailedMigrationKeepsLegacyEntryAndCachedPassword)
{
    m_collection.createItemShouldFail = true;

    TestableSettings settings;

    EXPECT_EQ(settings.password(), QStringLiteral("legacy-plaintext-password"));

    QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
    EXPECT_TRUE(raw.contains("password"));
}

TEST_F(SettingsPasswordMigrationTest, ExistingCachedPasswordDropsLegacyEntryWithoutMigrating)
{
    m_service.reportExistingItem = true;

    TestableSettings settings;

    EXPECT_EQ(settings.password(), QStringLiteral("existing-credential-store-password"));

    QSettings raw(QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime");
    EXPECT_FALSE(raw.contains("password"));
}

#endif // defined(Q_OS_LINUX)
