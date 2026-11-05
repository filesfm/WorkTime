#include <gtest/gtest.h>

#include <QByteArray>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusVariant>
#include <QDBusVirtualObject>
#include <QList>
#include <QString>
#include <QTest>
#include <QVariant>

#include "core/settings/credentialstore.hpp"

namespace {

class TestableCredentialStore : public CredentialStore
{
public:
    using CredentialStore::s_defaultCollectionPath;
    using CredentialStore::s_secretServiceName;
    using CredentialStore::s_secretServicePath;
};

constexpr auto kServicePath = "/org/freedesktop/secrets";
constexpr auto kCollectionPath = "/org/freedesktop/secrets/aliases/default";
constexpr auto kItemPath = "/org/freedesktop/secrets/collection/c1/item1";
constexpr auto kSessionPath = "/org/freedesktop/secrets/session/s1";

// Stands in for the Secret Service's root object: OpenSession, SearchItems,
// Unlock. unlockedItems/lockedItems model the item's current lock state, and
// Unlock "resolves" lockedItems into unlockedItems so a subsequent
// SearchItems reflects it - matching the real service's behavior that
// findItem() relies on (unlock, then re-search).
class FakeSecretService : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    bool openSessionShouldFail = false;
    QDBusObjectPath sessionPath{QString::fromLatin1(kSessionPath)};

    QList<QDBusObjectPath> unlockedItems;
    QList<QDBusObjectPath> lockedItems;
    QDBusObjectPath unlockPromptPath{QStringLiteral("/")};
    bool unlockResolvesLockedItems = true;

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("OpenSession")) {
            if (openSessionShouldFail) {
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("no session")));
            } else {
                connection.send(message.createReply(
                    QList<QVariant>{QVariant::fromValue(QDBusVariant(QString())), QVariant::fromValue(sessionPath)}));
            }
        } else if (message.member() == QStringLiteral("SearchItems")) {
            connection.send(message.createReply(
                QList<QVariant>{QVariant::fromValue(unlockedItems), QVariant::fromValue(lockedItems)}));
        } else if (message.member() == QStringLiteral("Unlock")) {
            if (unlockResolvesLockedItems) {
                unlockedItems.append(lockedItems);
                lockedItems.clear();
            }
            connection.send(message.createReply(
                QList<QVariant>{QVariant::fromValue(unlockedItems), QVariant::fromValue(unlockPromptPath)}));
        }
        return true;
    }
};

// Stands in for the default collection: CreateItem (writePassword()).
class FakeCollection : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    bool createItemShouldFail = false;
    QDBusObjectPath createdItemPath{QString::fromLatin1(kItemPath)};
    QDBusObjectPath createItemPromptPath{QStringLiteral("/")};

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("CreateItem")) {
            if (createItemShouldFail) {
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("cannot create")));
            } else {
                connection.send(message.createReply(
                    QList<QVariant>{QVariant::fromValue(createdItemPath), QVariant::fromValue(createItemPromptPath)}));
            }
        }
        return true;
    }
};

// Stands in for a stored item: GetSecret (readPassword()), Delete (deletePassword()).
class FakeItem : public QDBusVirtualObject
{
public:
    QList<QDBusMessage> calls;

    bool getSecretShouldFail = false;
    QByteArray secretValue;
    bool deleteShouldFail = false;
    QDBusObjectPath deletePromptPath{QStringLiteral("/")};

    QString introspect(const QString &) const override { return QString(); }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("GetSecret")) {
            if (getSecretShouldFail) {
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("cannot get secret")));
                return true;
            }
            // The Secret Service "Secret" struct: (session, parameters, value, content_type).
            QDBusArgument secretArg;
            secretArg.beginStructure();
            secretArg << QDBusObjectPath(QString::fromLatin1(kSessionPath)) << QByteArray() << secretValue
                      << QStringLiteral("text/plain; charset=utf8");
            secretArg.endStructure();
            connection.send(message.createReply(QVariant::fromValue(secretArg)));
        } else if (message.member() == QStringLiteral("Delete")) {
            if (deleteShouldFail)
                connection.send(message.createErrorReply(QDBusError::Failed, QStringLiteral("cannot delete")));
            else
                connection.send(message.createReply(QVariant::fromValue(deletePromptPath)));
        }
        return true;
    }
};

// Stands in for an unlock prompt: resolves the service's locked items (as
// the real keyring daemon would once the user approves the dialog) and then
// emits Completed, which waitForPrompt()'s nested event loop is blocked on.
class FakePrompt : public QDBusVirtualObject
{
public:
    FakeSecretService *service = nullptr;
    QList<QDBusMessage> calls;

    // QDBusInterface's old-style SIGNAL()/SLOT() connect (as waitForPrompt()
    // uses) introspects the interface first, and silently refuses to connect
    // if the signal isn't declared there - a non-empty XML is required, not
    // just handling the Completed signal's delivery.
    QString introspect(const QString &) const override
    {
        return QStringLiteral("<interface name=\"org.freedesktop.Secret.Prompt\">"
                              "<signal name=\"Completed\">"
                              "<arg type=\"b\" direction=\"out\"/>"
                              "<arg type=\"v\" direction=\"out\"/>"
                              "</signal>"
                              "</interface>");
    }

    bool handleMessage(const QDBusMessage &message, const QDBusConnection &connection) override
    {
        if (message.interface() == QStringLiteral("org.freedesktop.DBus.Introspectable"))
            return false;
        calls.append(message);

        if (message.member() == QStringLiteral("Prompt")) {
            if (service) {
                service->unlockedItems.append(service->lockedItems);
                service->lockedItems.clear();
            }
            QDBusMessage signal = QDBusMessage::createSignal(message.path(),
                                                             QStringLiteral("org.freedesktop.Secret.Prompt"),
                                                             QStringLiteral("Completed"));
            signal << false << QVariant::fromValue(QDBusVariant(QString()));
            connection.send(signal);
        }
        return true;
    }
};

class CredentialStoreTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_bus = QDBusConnection::sessionBus();
        if (!m_bus.isConnected())
            GTEST_SKIP() << "no D-Bus session bus available";

        ASSERT_TRUE(m_bus.registerVirtualObject(QString::fromLatin1(kServicePath), &m_service));
        ASSERT_TRUE(m_bus.registerVirtualObject(QString::fromLatin1(kCollectionPath), &m_collection));
        ASSERT_TRUE(m_bus.registerVirtualObject(QString::fromLatin1(kItemPath), &m_item));

        m_previousServiceName = TestableCredentialStore::s_secretServiceName;
        m_previousServicePath = TestableCredentialStore::s_secretServicePath;
        m_previousCollectionPath = TestableCredentialStore::s_defaultCollectionPath;

        TestableCredentialStore::s_secretServiceName = m_bus.baseService();
        TestableCredentialStore::s_secretServicePath = QString::fromLatin1(kServicePath);
        TestableCredentialStore::s_defaultCollectionPath = QString::fromLatin1(kCollectionPath);
    }

    void TearDown() override
    {
        if (!m_bus.isConnected())
            return;

        TestableCredentialStore::s_secretServiceName = m_previousServiceName;
        TestableCredentialStore::s_secretServicePath = m_previousServicePath;
        TestableCredentialStore::s_defaultCollectionPath = m_previousCollectionPath;

        m_bus.unregisterObject(QString::fromLatin1(kServicePath));
        m_bus.unregisterObject(QString::fromLatin1(kCollectionPath));
        m_bus.unregisterObject(QString::fromLatin1(kItemPath));
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

TEST_F(CredentialStoreTest, ReadPasswordReturnsEmptyWhenNoItemExists)
{
    EXPECT_EQ(CredentialStore::readPassword(), QString());
}

TEST_F(CredentialStoreTest, ReadPasswordReturnsStoredValueWhenItemIsUnlocked)
{
    m_service.unlockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_item.secretValue = QByteArrayLiteral("s3cret");

    EXPECT_EQ(CredentialStore::readPassword(), QStringLiteral("s3cret"));
}

TEST_F(CredentialStoreTest, ReadPasswordReturnsEmptyWhenOpenSessionFails)
{
    m_service.unlockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_service.openSessionShouldFail = true;

    EXPECT_EQ(CredentialStore::readPassword(), QString());
}

TEST_F(CredentialStoreTest, ReadPasswordReturnsEmptyWhenGetSecretFails)
{
    m_service.unlockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_item.getSecretShouldFail = true;

    EXPECT_EQ(CredentialStore::readPassword(), QString());
}

TEST_F(CredentialStoreTest, ReadPasswordUnlocksALockedItemThenReturnsItsValue)
{
    m_service.lockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_service.unlockPromptPath = QDBusObjectPath(QStringLiteral("/")); // no real prompt needed
    m_item.secretValue = QByteArrayLiteral("unlocked-secret");

    EXPECT_EQ(CredentialStore::readPassword(), QStringLiteral("unlocked-secret"));

    bool sawUnlock = false;
    for (const QDBusMessage &call : m_service.calls) {
        if (call.member() == QStringLiteral("Unlock"))
            sawUnlock = true;
    }
    EXPECT_TRUE(sawUnlock);
}

TEST_F(CredentialStoreTest, ReadPasswordWaitsForARealUnlockPromptThenReturnsValue)
{
    const QString promptPath = QStringLiteral("/org/freedesktop/secrets/prompt/p1");
    FakePrompt prompt;
    prompt.service = &m_service;
    ASSERT_TRUE(m_bus.registerVirtualObject(promptPath, &prompt));

    m_service.lockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_service.unlockResolvesLockedItems = false; // only the prompt resolves it, not Unlock's own reply
    m_service.unlockPromptPath = QDBusObjectPath(promptPath);
    m_item.secretValue = QByteArrayLiteral("prompted-secret");

    EXPECT_EQ(CredentialStore::readPassword(), QStringLiteral("prompted-secret"));
    EXPECT_FALSE(prompt.calls.isEmpty());

    m_bus.unregisterObject(promptPath);
}

TEST_F(CredentialStoreTest, WritePasswordReturnsFalseWhenOpenSessionFails)
{
    m_service.openSessionShouldFail = true;

    EXPECT_FALSE(CredentialStore::writePassword(QStringLiteral("pw")));
    EXPECT_TRUE(m_collection.calls.isEmpty());
}

TEST_F(CredentialStoreTest, WritePasswordReturnsFalseWhenCreateItemFails)
{
    m_collection.createItemShouldFail = true;

    EXPECT_FALSE(CredentialStore::writePassword(QStringLiteral("pw")));
}

TEST_F(CredentialStoreTest, WritePasswordReturnsTrueOnSuccess)
{
    EXPECT_TRUE(CredentialStore::writePassword(QStringLiteral("pw")));

    bool sawCreateItem = false;
    for (const QDBusMessage &call : m_collection.calls) {
        if (call.member() == QStringLiteral("CreateItem"))
            sawCreateItem = true;
    }
    EXPECT_TRUE(sawCreateItem);
}

TEST_F(CredentialStoreTest, DeletePasswordReturnsTrueWhenNoItemExists)
{
    EXPECT_TRUE(CredentialStore::deletePassword());
    EXPECT_TRUE(m_item.calls.isEmpty());
}

TEST_F(CredentialStoreTest, DeletePasswordReturnsFalseWhenDeleteFails)
{
    m_service.unlockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};
    m_item.deleteShouldFail = true;

    EXPECT_FALSE(CredentialStore::deletePassword());
}

TEST_F(CredentialStoreTest, DeletePasswordReturnsTrueOnSuccess)
{
    m_service.unlockedItems = {QDBusObjectPath(QString::fromLatin1(kItemPath))};

    EXPECT_TRUE(CredentialStore::deletePassword());

    bool sawDelete = false;
    for (const QDBusMessage &call : m_item.calls) {
        if (call.member() == QStringLiteral("Delete"))
            sawDelete = true;
    }
    EXPECT_TRUE(sawDelete);
}
