#include "settings.hpp"

#include <QList>
#include <QVariant>

#include <functional>

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
    //   postInterval        - seconds, must be > 0
    //   enableShots         - true or false
    //   shotScale           - fraction of full resolution, (0.0, 1.0]
    //   shotCompression     - percentage quality, [0, 100]
    const QList<SettingSpec> specs{
        {"username", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
        {"password", QString(), [](const QVariant &v) { return v.canConvert<QString>(); }},
        {"autoStartup", false, [](const QVariant &v) { return v.canConvert<bool>(); }},
        {"postInterval",
         30,
         [](const QVariant &v) {
             bool ok = false;
             const int interval = v.toInt(&ok);
             return ok && interval > 0;
         }},
        {"enableShots", false, [](const QVariant &v) { return v.canConvert<bool>(); }},
        {"shotScale",
         1.0,
         [](const QVariant &v) {
             bool ok = false;
             const qreal scale = v.toDouble(&ok);
             return ok && scale > 0.0 && scale <= 1.0;
         }},
        {"shotCompression",
         80,
         [](const QVariant &v) {
             bool ok = false;
             const int compression = v.toInt(&ok);
             return ok && compression >= 0 && compression <= 100;
         }},
    };

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
    emit autoStartupChanged();
}

int Settings::postInterval() const
{
    return m_settings.value("postInterval").toInt();
}

void Settings::setPostInterval(int seconds)
{
    if (postInterval() == seconds)
        return;
    m_settings.setValue("postInterval", seconds);
    emit postIntervalChanged();
}

bool Settings::enableShots() const
{
    return m_settings.value("enableShots").toBool();
}

void Settings::setEnableShots(bool enabled)
{
    if (enableShots() == enabled)
        return;
    m_settings.setValue("enableShots", enabled);
    emit enableShotsChanged();
}

qreal Settings::shotScale() const
{
    return m_settings.value("shotScale").toReal();
}

void Settings::setShotScale(qreal scale)
{
    if (qFuzzyCompare(shotScale(), scale))
        return;
    m_settings.setValue("shotScale", scale);
    emit shotScaleChanged();
}

int Settings::shotCompression() const
{
    return m_settings.value("shotCompression").toInt();
}

void Settings::setShotCompression(int compression)
{
    if (shotCompression() == compression)
        return;
    m_settings.setValue("shotCompression", compression);
    emit shotCompressionChanged();
}
