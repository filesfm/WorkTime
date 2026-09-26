#include <gtest/gtest.h>
#include <type_traits>

#include "core/kdefocusedwindowtitle.hpp"

TEST(KDEFocusedWindowTitleTest, IsNotCopyableOrMovable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_copy_assignable_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_move_constructible_v<KDEFocusedWindowTitle>);
    EXPECT_FALSE(std::is_move_assignable_v<KDEFocusedWindowTitle>);
}

TEST(KDEFocusedWindowTitleTest, InstanceReturnsNonNullPointer)
{
    EXPECT_NE(KDEFocusedWindowTitle::instance(), nullptr);
}

TEST(KDEFocusedWindowTitleTest, InstanceReturnsSameInstanceOnRepeatedCalls)
{
    EXPECT_EQ(KDEFocusedWindowTitle::instance(), KDEFocusedWindowTitle::instance());
}

// This must run before any test that calls setActiveWindowTitle().
TEST(KDEFocusedWindowTitleTest, ActiveWindowTitleIsEmptyBeforeAnyTitleIsSet)
{
    EXPECT_TRUE(KDEFocusedWindowTitle::instance()->activeWindowTitle().isEmpty());
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleUpdatesActiveWindowTitle)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Editor - file.cpp"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("Editor - file.cpp"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleOverwritesPreviousValue)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("First Window"));
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Second Window"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("Second Window"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleAcceptsEmptyString)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("Some Window"));
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QString());

    EXPECT_TRUE(KDEFocusedWindowTitle::instance()->activeWindowTitle().isEmpty());
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleHandlesUnicodeCharacters)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("日本語 — emoji 🎉 window"));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("日本語 — emoji 🎉 window"));
}

TEST(KDEFocusedWindowTitleTest, SetActiveWindowTitleDoesNotTrimWhitespace)
{
    KDEFocusedWindowTitle::instance()->setActiveWindowTitle(QStringLiteral("  padded title  "));

    EXPECT_EQ(KDEFocusedWindowTitle::instance()->activeWindowTitle(), QStringLiteral("  padded title  "));
}
