#include <gtest/gtest.h>

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>
#include <QVariant>

#include "core/settings.hpp"

namespace {

class SettingsEnvironment : public ::testing::Environment
{
public:
    void SetUp() override
    {
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
