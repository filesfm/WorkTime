#include "settings.hpp"

#include <QHash>
#include <QList>
#include <QVariant>

#include <functional>

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
//   username, password  - no constraint, any string (including empty)
//   autoStartup         - true or false
//   startButtonPushed   - true or false
const QList<SettingSpec> kSpecs{{"username", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
                                {"password", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
                                {"autoStartup", false, [](const QVariant &v) { return v.canConvert<bool>(); }},
                                {"startButtonPushed", false, [](const QVariant &v) { return v.canConvert<bool>(); }}};

} // namespace

Settings::Settings(QObject *parent)
    : QObject{parent}
{
    qCInfo(worktimeSettings) << "loading settings from" << m_settings.fileName();

    sanitize();
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
    return m_settings.value("password").toString();
}

void Settings::setPassword(const QString &password)
{
    qCDebug(worktimeSettings) << "setPassword called";
    if (this->password() == password)
        return;
    qCInfo(worktimeSettings) << "password changed";
    m_settings.setValue("password", password);
    emit passwordChanged();
}

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
