#include <gtest/gtest.h>
#include <type_traits>

#include "core/oneinstanceguarantor.hpp"

TEST(OneInstanceGuarantorTest, IsNotCopyable)
{
    EXPECT_FALSE(std::is_copy_constructible_v<OneInstanceGuarantor>);
    EXPECT_FALSE(std::is_copy_assignable_v<OneInstanceGuarantor>);
}

TEST(OneInstanceGuarantorTest, FirstInstanceAcquiresLock)
{
    EXPECT_NO_THROW(OneInstanceGuarantor guard{});
}

TEST(OneInstanceGuarantorTest, SecondInstanceThrowsWhileFirstIsAlive)
{
    OneInstanceGuarantor first{};
    EXPECT_THROW(OneInstanceGuarantor second{}, std::runtime_error);
}

TEST(OneInstanceGuarantorTest, LockIsReleasedOnDestruction)
{
    {
        OneInstanceGuarantor first{};
    }
    EXPECT_NO_THROW(OneInstanceGuarantor second{});
}

TEST(OneInstanceGuarantorTest, ThrowsWithExpectedMessage)
{
    OneInstanceGuarantor first{};
    try {
        OneInstanceGuarantor second{};
        FAIL();
    } catch (const std::runtime_error &e) {
        EXPECT_STREQ(e.what(), "One program instance already exists.");
    }
}

TEST(OneInstanceGuarantorTest, FirstInstanceRemainsLockedAfterFailedSecondAttempt)
{
    OneInstanceGuarantor first{};

    EXPECT_THROW(OneInstanceGuarantor second{}, std::runtime_error);

    // The failed attempt above must not have disturbed first's lock.
    EXPECT_THROW(OneInstanceGuarantor third{}, std::runtime_error);
}

TEST(OneInstanceGuarantorTest, SupportsRepeatedAcquireReleaseCycles)
{
    for (int i{0}; i < 5; ++i) {
        EXPECT_NO_THROW(OneInstanceGuarantor guard{});
    }
}

namespace {

class TestableOneInstanceGuarantor : public OneInstanceGuarantor
{
public:
    using OneInstanceGuarantor::lockFilePath;

    bool isLocked() const { return m_lockFile.isLocked(); }
    QString fileName() const { return m_lockFile.fileName(); }
};

} // namespace

TEST(OneInstanceGuarantorTest, LockFileIsActuallyLockedAfterConstruction)
{
    TestableOneInstanceGuarantor guard{};

    EXPECT_TRUE(guard.isLocked());
    EXPECT_EQ(guard.fileName(), TestableOneInstanceGuarantor::lockFilePath());
}
