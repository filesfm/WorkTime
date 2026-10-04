#include "settings.hpp"

#include <QHash>
#include <QList>
#include <QVariant>

#include <functional>

#include "core/settings/credentialstore.hpp"
#include "core/utilities.hpp"

Q_LOGGING_CATEGORY(worktimeSettings, "worktime.settings")

namespace {

struct SettingSpec
{
    QString key;
    QVariant defaultValue;
    std::function<bool(const QVariant &)> isValid;
};

// Invariants:
//   username            - no constraint, any string (including empty)
//   autoStartup         - true or false
//   startButtonPushed   - true or false
// password isn't listed here: it lives in the OS credential store (see
// CredentialStore/loadPassword()), not among these QSettings-backed keys.
const QList<SettingSpec> kSpecs{{"username", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
#if !defined(BUILD_WITHOUT_AUTOSTART)
                                {"autoStartup", false, [](const QVariant &v) { return v.canConvert<bool>(); }},
#endif
                                {"startButtonPushed", false, [](const QVariant &v) { return v.canConvert<bool>(); }}};

} // namespace

Settings::Settings(QObject *parent)
    : QObject{parent}
{
    qCInfo(worktimeSettings) << "loading settings from" << m_settings.fileName();

    sanitize();
    loadPassword();
}

Settings *Settings::instance()
{
    static Settings s;
    return &s;
}

QString Settings::username() const
{
    return m_settings.value("username").toString();
}

void Settings::setUsername(const QString &username)
{
    qCDebug(worktimeSettings) << "setUsername called with" << username;
    if (this->username() == username)
        return;
    qCInfo(worktimeSettings) << "username changed to" << username;
    m_settings.setValue("username", username);
    emit usernameChanged();
}

QString Settings::password() const
{
    return m_cachedPassword;
}

void Settings::setPassword(const QString &password)
{
    qCDebug(worktimeSettings) << "setPassword called";
    if (m_cachedPassword == password)
        return;
    qCInfo(worktimeSettings) << "password changed";
    if (!CredentialStore::writePassword(password))
        qCWarning(worktimeSettings) << "failed to write password to the OS credential store";
    m_cachedPassword = password;
    emit passwordChanged();
}

#if !defined(BUILD_WITHOUT_AUTOSTART)

bool Settings::autoStartup() const
{
    return m_settings.value("autoStartup").toBool();
}

void Settings::setAutoStartup(bool enabled)
{
    qCDebug(worktimeSettings) << "setAutoStartup called with" << enabled;
    if (autoStartup() == enabled)
        return;
    qCInfo(worktimeSettings) << "autoStartup changed to" << enabled;
    m_settings.setValue("autoStartup", enabled);
    Utilities::autostart(enabled);
    emit autoStartupChanged();
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART)

bool Settings::startButtonPushed() const
{
    return m_settings.value("startButtonPushed").toBool();
}

void Settings::setStartButtonPushed(bool pushed)
{
    qCDebug(worktimeSettings) << "setStartButtonPushed called with" << pushed;
    if (startButtonPushed() == pushed)
        return;
    qCInfo(worktimeSettings) << "startButtonPushed changed to" << pushed;
    m_settings.setValue("startButtonPushed", pushed);
    emit startButtonChanged();
}

void Settings::sanitize()
{
    if (m_settings.status() != QSettings::NoError) {
        qCCritical(worktimeSettings) << "failed to load settings from" << m_settings.fileName() << "(status"
                                     << m_settings.status() << ")";
        if (m_settings.status() == QSettings::FormatError) {
            qCCritical(worktimeSettings) << "settings file is malformed, resetting to defaults";
            m_settings.clear();
        } else if (m_settings.status() == QSettings::AccessError) {
            qCFatal(worktimeSettings) << "settings file is not readable/writable at" << m_settings.fileName();
        }
    }

    QHash<QString, QVariant> defaults;
    for (const auto &spec : kSpecs)
        defaults.insert(spec.key, spec.defaultValue);

    for (const QString &key : m_settings.allKeys()) {
        // A legacy plaintext password left over from before CredentialStore
        // existed; loadPassword() migrates and removes it, not this loop.
        if (key == QLatin1String("password"))
            continue;
        if (!defaults.contains(key)) {
            qCInfo(worktimeSettings) << "deleting unknown option" << key;
            m_settings.remove(key);
        }
    }

    for (const auto &spec : kSpecs) {
        if (!m_settings.contains(spec.key) || !spec.isValid(m_settings.value(spec.key))) {
            qCInfo(worktimeSettings) << "resetting" << spec.key << "to its default value";
            m_settings.setValue(spec.key, spec.defaultValue);
        }
    }
}

void Settings::loadPassword()
{
    m_cachedPassword = CredentialStore::readPassword();

    const QString legacyPassword = m_settings.value("password").toString();
    if (legacyPassword.isEmpty())
        return;

    if (!m_cachedPassword.isEmpty()) {
        // The credential store already has a password (e.g. a prior launch
        // migrated it successfully); just drop the now-redundant plaintext copy.
        m_settings.remove("password");
        return;
    }

    qCInfo(worktimeSettings) << "migrating password from plaintext settings to the OS credential store";
    if (CredentialStore::writePassword(legacyPassword)) {
        m_cachedPassword = legacyPassword;
        m_settings.remove("password");
    } else {
        qCWarning(worktimeSettings)
            << "failed to migrate password to the OS credential store; keeping it in Settings for now";
        m_cachedPassword = legacyPassword;
    }
}
