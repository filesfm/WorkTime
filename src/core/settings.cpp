#include "settings.hpp"

#include <QList>
#include <QVariant>

#include <functional>

#include "core/utilities.hpp"

namespace {

struct SettingSpec
{
    QString key;
    QVariant defaultValue;
    std::function<bool(const QVariant &)> isValid;
};

} // namespace

Settings::Settings(QObject *parent)
    : QObject{parent}
    , m_settings{QSettings::NativeFormat, QSettings::UserScope, "Files.fm", "Worktime"}
{
    // Invariants:
    //   username, password  - no constraint, any string (including empty)
    //   autoStartup         - true or false
    //   startButtonPushed   - true or false
    const QList<SettingSpec> specs{{"username", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
                                   {"password", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
                                   {"autoStartup", false, [](const QVariant &v) { return v.canConvert<bool>(); }},
                                   {"startButtonPushed", false, [](const QVariant &v) { return v.canConvert<bool>(); }}};

    for (const auto &spec : specs) {
        if (!m_settings.contains(spec.key) || !spec.isValid(m_settings.value(spec.key)))
            m_settings.setValue(spec.key, spec.defaultValue);
    }
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
    if (this->username() == username)
        return;
    m_settings.setValue("username", username);
    emit usernameChanged();
}

QString Settings::password() const
{
    return m_settings.value("password").toString();
}

void Settings::setPassword(const QString &password)
{
    if (this->password() == password)
        return;
    m_settings.setValue("password", password);
    emit passwordChanged();
}

bool Settings::autoStartup() const
{
    return m_settings.value("autoStartup").toBool();
}

void Settings::setAutoStartup(bool enabled)
{
    if (autoStartup() == enabled)
        return;
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
    if (startButtonPushed() == pushed)
        return;
    m_settings.setValue("startButtonPushed", pushed);
    emit startButtonChanged();
}
