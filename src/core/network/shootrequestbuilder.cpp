#include "shootrequestbuilder.hpp"

#include "core/settings/settings.hpp"
#include "core/utilities.hpp"

#include <QUrl>

namespace {

constexpr qsizetype kMaxMetadataLength = 255;

QByteArray encodeField(const QString &key, const QString &value)
{
    return QUrl::toPercentEncoding(key) + '=' + QUrl::toPercentEncoding(value);
}

} // namespace

QByteArray buildShootRequestBody(const QString &title, qint64 utcTimestamp, qint64 shootTime)
{
    QByteArray body = encodeField(QStringLiteral("Shoot[user_name]"), Settings::instance()->username()) + '&'
                      + encodeField(QStringLiteral("Shoot[password]"), Settings::instance()->password()) + '&'
                      + encodeField(QStringLiteral("Shoot[project_name]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[app_name]"),
                                    Utilities::truncateUtf8Safe(title, kMaxMetadataLength))
                      + '&' + encodeField(QStringLiteral("Shoot[document_name]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[document_path]"), QString()) + '&'
                      + encodeField(QStringLiteral("Shoot[shoot_time]"), QString::number(shootTime)) + '&'
                      + encodeField(QStringLiteral("Shoot[utc_timestamp]"), QString::number(utcTimestamp)) + '&'
                      + encodeField(QStringLiteral("Shoot[image_resized]"), QStringLiteral("1"));

    return body;
}
