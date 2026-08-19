#include <gtest/gtest.h>

#include <QSettings>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QString>

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
    EXPECT_EQ(settings->postInterval(), 30);
    EXPECT_FALSE(settings->enableShots());
    EXPECT_DOUBLE_EQ(settings->shotScale(), 1.0);
    EXPECT_EQ(settings->shotCompression(), 80);
}

TEST(SettingsTest, UsernameRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setUsername("alice");
    EXPECT_EQ(settings->username(), QStringLiteral("alice"));
}

TEST(SettingsTest, PasswordRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setPassword("hunter2");
    EXPECT_EQ(settings->password(), QStringLiteral("hunter2"));
}

TEST(SettingsTest, AutoStartupRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setAutoStartup(true);
    EXPECT_TRUE(settings->autoStartup());
    settings->setAutoStartup(false);
    EXPECT_FALSE(settings->autoStartup());
}

TEST(SettingsTest, PostIntervalRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setPostInterval(600);
    EXPECT_EQ(settings->postInterval(), 600);
}

TEST(SettingsTest, EnableShotsRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setEnableShots(true);
    EXPECT_TRUE(settings->enableShots());
    settings->setEnableShots(false);
    EXPECT_FALSE(settings->enableShots());
}

TEST(SettingsTest, ShotScaleRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setShotScale(0.5);
    EXPECT_DOUBLE_EQ(settings->shotScale(), 0.5);
}

TEST(SettingsTest, ShotCompressionRoundTrips)
{
    Settings *settings = Settings::instance();
    settings->setShotCompression(50);
    EXPECT_EQ(settings->shotCompression(), 50);
}
