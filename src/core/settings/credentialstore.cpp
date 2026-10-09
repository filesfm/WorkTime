#include "credentialstore.hpp"

#include <QGlobalStatic>
#include <QMutex>
#include <QMutexLocker>

#if defined(Q_OS_LINUX)
#    include <QDBusArgument>
#    include <QDBusConnection>
#    include <QDBusInterface>
#    include <QDBusMessage>
#    include <QDBusMetaType>
#    include <QDBusObjectPath>
#    include <QDBusVariant>
#    include <QEventLoop>
#    include <QMap>
#elif defined(Q_OS_MACOS)
#    include <Security/Security.h>
#elif defined(Q_OS_WINDOWS)
#    include <windows.h>

#    include <wincred.h>
#endif

Q_LOGGING_CATEGORY(worktimeCredentialStore, "worktime.credential.store")

namespace {
constexpr auto kApplicationId = "io.github.filesfm.worktime";
} // namespace

Q_GLOBAL_STATIC(QMutex, credentialStoreMutex)

#if defined(Q_OS_LINUX)

namespace {

constexpr auto kSecretServiceName = "org.freedesktop.secrets";
constexpr auto kSecretServicePath = "/org/freedesktop/secrets";
constexpr auto kDefaultCollectionPath = "/org/freedesktop/secrets/aliases/default";
constexpr auto kApplicationAttribute = "application";

// The Secret Service "Secret" struct: (session, parameters, value, content_type).
struct SecretServiceSecret
{
    QDBusObjectPath session;
    QByteArray parameters;
    QByteArray value;
    QString contentType;
};

QDBusArgument &operator<<(QDBusArgument &arg, const SecretServiceSecret &secret)
{
    arg.beginStructure();
    arg << secret.session << secret.parameters << secret.value << secret.contentType;
    arg.endStructure();
    return arg;
}

const QDBusArgument &operator>>(const QDBusArgument &arg, SecretServiceSecret &secret)
{
    arg.beginStructure();
    arg >> secret.session >> secret.parameters >> secret.value >> secret.contentType;
    arg.endStructure();
    return arg;
}

// Builds a D-Bus a{ss} dict. A plain QVariantMap marshals as a{sv}, which
// the Secret Service spec requires for item *properties* but rejects for
// item *attributes* (must be a{ss}), so that one value needs building by
// hand via QDBusArgument.
QDBusArgument stringDict(const QMap<QString, QString> &map)
{
    QDBusArgument arg;
    arg.beginMap(QMetaType::QString, QMetaType::QString);
    for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
        arg.beginMapEntry();
        arg << it.key() << it.value();
        arg.endMapEntry();
    }
    arg.endMap();
    return arg;
}

QMap<QString, QString> itemAttributes()
{
    return {{kApplicationAttribute, kApplicationId}};
}

QDBusInterface serviceInterface()
{
    return QDBusInterface(kSecretServiceName,
                          kSecretServicePath,
                          "org.freedesktop.Secret.Service",
                          QDBusConnection::sessionBus());
}

// Blocks (via a local event loop) until the prompt at promptPath completes -
// e.g. because unlocking the keyring required a user-facing dialog. A no-op
// if promptPath is "/", the well-known path meaning no prompt is needed,
// which is the common case for an already-unlocked login keyring.
void waitForPrompt(const QDBusObjectPath &promptPath)
{
    if (promptPath.path().isEmpty() || promptPath.path() == QLatin1String("/"))
        return;

    QDBusInterface prompt(kSecretServiceName,
                          promptPath.path(),
                          "org.freedesktop.Secret.Prompt",
                          QDBusConnection::sessionBus());
    QEventLoop loop;
    QObject::connect(&prompt, SIGNAL(Completed(bool, QDBusVariant)), &loop, SLOT(quit()));
    prompt.call(QDBus::NoBlock, "Prompt", QString());
    loop.exec();
}

// Opens a "plain" (unencrypted) Secret Service session. "plain" is
// explicitly permitted by the spec and avoids needing a crypto
// implementation here: the transport is local D-Bus, not a network channel.
QDBusObjectPath openSession()
{
    QDBusMessage reply = serviceInterface().call("OpenSession",
                                                 QStringLiteral("plain"),
                                                 QVariant::fromValue(QDBusVariant(QString())));
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
        qCWarning(worktimeCredentialStore) << "failed to open Secret Service session:" << reply.errorMessage();
        return QDBusObjectPath();
    }
    return qvariant_cast<QDBusObjectPath>(reply.arguments().at(1));
}

// Searches every collection for this app's item, unlocking it first if
// needed. Returns the item's object path, or an empty path if none exists.
QDBusObjectPath findItem()
{
    QDBusMessage searchReply = serviceInterface().call("SearchItems", QVariant::fromValue(stringDict(itemAttributes())));
    if (searchReply.type() != QDBusMessage::ReplyMessage || searchReply.arguments().size() < 2) {
        qCWarning(worktimeCredentialStore) << "SearchItems failed:" << searchReply.errorMessage();
        return QDBusObjectPath();
    }

    QList<QDBusObjectPath> unlocked = qdbus_cast<QList<QDBusObjectPath>>(searchReply.arguments().at(0));
    const QList<QDBusObjectPath> locked = qdbus_cast<QList<QDBusObjectPath>>(searchReply.arguments().at(1));

    if (unlocked.isEmpty() && !locked.isEmpty()) {
        QDBusMessage unlockReply = serviceInterface().call("Unlock", QVariant::fromValue(locked));
        if (unlockReply.type() == QDBusMessage::ReplyMessage && unlockReply.arguments().size() >= 2)
            waitForPrompt(qvariant_cast<QDBusObjectPath>(unlockReply.arguments().at(1)));

        // Re-search rather than parse the prompt's result: simpler, and
        // correct whether the item ended up unlocked via the initial reply
        // or via a completed prompt.
        searchReply = serviceInterface().call("SearchItems", QVariant::fromValue(stringDict(itemAttributes())));
        if (searchReply.type() == QDBusMessage::ReplyMessage && !searchReply.arguments().isEmpty())
            unlocked = qdbus_cast<QList<QDBusObjectPath>>(searchReply.arguments().at(0));
    }

    return unlocked.isEmpty() ? QDBusObjectPath() : unlocked.first();
}

} // namespace

QString CredentialStore::readPassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    qDBusRegisterMetaType<SecretServiceSecret>();

    const QDBusObjectPath itemPath = findItem();
    if (itemPath.path().isEmpty()) {
        qCDebug(worktimeCredentialStore) << "no stored credential found";
        return QString();
    }

    const QDBusObjectPath session = openSession();
    if (session.path().isEmpty())
        return QString();

    QDBusInterface item(kSecretServiceName,
                        itemPath.path(),
                        "org.freedesktop.Secret.Item",
                        QDBusConnection::sessionBus());
    QDBusMessage secretReply = item.call("GetSecret", QVariant::fromValue(session));
    if (secretReply.type() != QDBusMessage::ReplyMessage || secretReply.arguments().isEmpty()) {
        qCWarning(worktimeCredentialStore) << "GetSecret failed:" << secretReply.errorMessage();
        return QString();
    }

    const SecretServiceSecret secret = qdbus_cast<SecretServiceSecret>(secretReply.arguments().at(0));
    return QString::fromUtf8(secret.value);
}

bool CredentialStore::writePassword(const QString &password)
{
    const QMutexLocker locker(credentialStoreMutex());

    qDBusRegisterMetaType<SecretServiceSecret>();

    const QDBusObjectPath session = openSession();
    if (session.path().isEmpty())
        return false;

    QVariantMap properties;
    properties[QStringLiteral("org.freedesktop.Secret.Item.Label")] = QStringLiteral("WorkTime");
    properties[QStringLiteral("org.freedesktop.Secret.Item.Attributes")] = QVariant::fromValue(
        stringDict(itemAttributes()));

    SecretServiceSecret secret;
    secret.session = session;
    secret.value = password.toUtf8();
    secret.contentType = QStringLiteral("text/plain; charset=utf8");

    QDBusInterface collection(kSecretServiceName,
                              kDefaultCollectionPath,
                              "org.freedesktop.Secret.Collection",
                              QDBusConnection::sessionBus());
    QDBusMessage reply = collection.call("CreateItem", properties, QVariant::fromValue(secret), true);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
        qCWarning(worktimeCredentialStore) << "CreateItem failed:" << reply.errorMessage();
        return false;
    }

    waitForPrompt(qvariant_cast<QDBusObjectPath>(reply.arguments().at(1)));
    return true;
}

bool CredentialStore::deletePassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    qDBusRegisterMetaType<SecretServiceSecret>();

    const QDBusObjectPath itemPath = findItem();
    if (itemPath.path().isEmpty())
        return true;

    QDBusInterface item(kSecretServiceName,
                        itemPath.path(),
                        "org.freedesktop.Secret.Item",
                        QDBusConnection::sessionBus());
    QDBusMessage reply = item.call("Delete");
    if (reply.type() != QDBusMessage::ReplyMessage) {
        qCWarning(worktimeCredentialStore) << "Delete failed:" << reply.errorMessage();
        return false;
    }

    if (!reply.arguments().isEmpty())
        waitForPrompt(qvariant_cast<QDBusObjectPath>(reply.arguments().at(0)));
    return true;
}

#elif defined(Q_OS_MACOS)

namespace {

CFStringRef toCFString(const QString &s)
{
    return CFStringCreateWithCharacters(kCFAllocatorDefault, reinterpret_cast<const UniChar *>(s.utf16()), s.length());
}

// Builds the attribute dictionary identifying our single keychain item,
// shared by read/write/delete so they all address the same entry.
CFMutableDictionaryRef baseQuery()
{
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                             0,
                                                             &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);

    CFStringRef service = toCFString(QString::fromLatin1(kApplicationId));
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, service);
    CFRelease(service);

    return query;
}

} // namespace

QString CredentialStore::readPassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    CFMutableDictionaryRef query = baseQuery();
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);

    CFTypeRef result = nullptr;
    const OSStatus status = SecItemCopyMatching(query, &result);
    CFRelease(query);

    if (status != errSecSuccess || !result) {
        if (status != errSecItemNotFound)
            qCWarning(worktimeCredentialStore) << "SecItemCopyMatching failed with status" << status;
        return QString();
    }

    CFDataRef data = static_cast<CFDataRef>(result);
    const QString password = QString::fromUtf8(reinterpret_cast<const char *>(CFDataGetBytePtr(data)),
                                               CFDataGetLength(data));
    CFRelease(result);
    return password;
}

bool CredentialStore::writePassword(const QString &password)
{
    const QMutexLocker locker(credentialStoreMutex());

    const QByteArray passwordUtf8 = password.toUtf8();
    CFDataRef data = CFDataCreate(kCFAllocatorDefault,
                                  reinterpret_cast<const UInt8 *>(passwordUtf8.constData()),
                                  passwordUtf8.size());

    CFMutableDictionaryRef query = baseQuery();
    OSStatus status = SecItemCopyMatching(query, nullptr);

    if (status == errSecItemNotFound) {
        CFDictionarySetValue(query, kSecValueData, data);
        status = SecItemAdd(query, nullptr);
    } else if (status == errSecSuccess) {
        CFMutableDictionaryRef update = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                  0,
                                                                  &kCFTypeDictionaryKeyCallBacks,
                                                                  &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(update, kSecValueData, data);
        status = SecItemUpdate(query, update);
        CFRelease(update);
    }

    CFRelease(data);
    CFRelease(query);

    if (status != errSecSuccess) {
        qCWarning(worktimeCredentialStore) << "failed to write to Keychain, status" << status;
        return false;
    }
    return true;
}

bool CredentialStore::deletePassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    CFMutableDictionaryRef query = baseQuery();
    const OSStatus status = SecItemDelete(query);
    CFRelease(query);

    if (status != errSecSuccess && status != errSecItemNotFound) {
        qCWarning(worktimeCredentialStore) << "failed to delete from Keychain, status" << status;
        return false;
    }
    return true;
}

#elif defined(Q_OS_WINDOWS)

namespace {
std::wstring targetName()
{
    return QString::fromLatin1(kApplicationId).toStdWString();
}
} // namespace

QString CredentialStore::readPassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    PCREDENTIALW credential = nullptr;
    if (!CredReadW(targetName().c_str(), CRED_TYPE_GENERIC, 0, &credential)) {
        if (GetLastError() != ERROR_NOT_FOUND)
            qCWarning(worktimeCredentialStore) << "CredReadW failed with error" << GetLastError();
        return QString();
    }

    QString password;
    if (credential->CredentialBlob && credential->CredentialBlobSize > 0) {
        password = QString::fromUtf16(reinterpret_cast<const char16_t *>(credential->CredentialBlob),
                                      credential->CredentialBlobSize / sizeof(wchar_t));
    }
    CredFree(credential);
    return password;
}

bool CredentialStore::writePassword(const QString &password)
{
    const QMutexLocker locker(credentialStoreMutex());

    const std::wstring target = targetName();
    const std::wstring secret = password.toStdWString();

    CREDENTIALW credential = {};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(target.c_str());
    credential.CredentialBlobSize = static_cast<DWORD>(secret.size() * sizeof(wchar_t));
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<wchar_t *>(secret.c_str()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;

    if (!CredWriteW(&credential, 0)) {
        qCWarning(worktimeCredentialStore) << "CredWriteW failed with error" << GetLastError();
        return false;
    }
    return true;
}

bool CredentialStore::deletePassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    if (!CredDeleteW(targetName().c_str(), CRED_TYPE_GENERIC, 0) && GetLastError() != ERROR_NOT_FOUND) {
        qCWarning(worktimeCredentialStore) << "CredDeleteW failed with error" << GetLastError();
        return false;
    }
    return true;
}

#else

QString CredentialStore::readPassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    qCWarning(worktimeCredentialStore) << "no credential store implementation for this platform";
    return QString();
}

bool CredentialStore::writePassword(const QString &)
{
    const QMutexLocker locker(credentialStoreMutex());

    qCWarning(worktimeCredentialStore) << "no credential store implementation for this platform";
    return false;
}

bool CredentialStore::deletePassword()
{
    const QMutexLocker locker(credentialStoreMutex());

    return true;
}

#endif
