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
