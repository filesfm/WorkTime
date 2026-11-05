#include "popupplacement.hpp"

#include <QGuiApplication>

#if defined(Q_OS_LINUX)
#    include "core/kwinscript.hpp"
#    include "core/utilities.hpp"

#    include <QDBusConnection>
#    include <QDBusMessage>
#    include <QDBusReply>
#    include <QDir>
#    include <QFile>
#    include <QStandardPaths>

namespace {

constexpr QLatin1StringView kGnomeExtensionUuid{"worktime-placement@filesfm.github.io"};
constexpr QLatin1StringView kKWinPluginName{"io.github.filesfm.worktime.popupplacement"};

struct ExtensionFile
{
    const char *resourcePath;
    const char *fileName;
};

constexpr ExtensionFile kGnomeExtensionFiles[] = {
    {":/qt/qml/Worktime/worktime-placement-metadata.json", "metadata.json"},
    {":/qt/qml/Worktime/worktime-placement-extension.js", "extension.js"},
};

/*!
 * \brief Writes \a content to \a path unless the file already holds exactly that.
 * \return Whether the file was written.
 */
bool writeIfChanged(const QString &path, const QByteArray &content)
{
    {
        QFile existing(path);
        if (existing.open(QIODevice::ReadOnly) && existing.readAll() == content)
            return false;
    }

    QFile destination(path);
    if (!destination.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCWarning(worktimeUtilities) << "failed to open" << path << "for writing";
        return false;
    }
    destination.write(content);
    return true;
}

} // namespace

QString PopupPlacement::s_gnomeShellExtensionsService = QStringLiteral("org.gnome.Shell.Extensions");
QString PopupPlacement::s_gnomeShellExtensionsPath = QStringLiteral("/org/gnome/Shell/Extensions");
QString PopupPlacement::s_gnomePlacementService = QStringLiteral("org.gnome.Shell");
QString PopupPlacement::s_gnomePlacementPath = QStringLiteral("/io/github/filesfm/worktime/Placement");
QString PopupPlacement::s_gnomePlacementInterface = QStringLiteral("io.github.filesfm.worktime.Placement");

#    if defined(BUILD_TESTING)
bool PopupPlacement::s_forceWaylandForTesting = false;
#    endif

bool PopupPlacement::isWayland()
{
#    if defined(BUILD_TESTING)
    if (s_forceWaylandForTesting)
        return true;
#    endif
    return QGuiApplication::platformName().startsWith(QLatin1String("wayland"));
}

bool PopupPlacement::desktopContains(const char *name)
{
    return qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QLatin1String(name), Qt::CaseInsensitive);
}

bool PopupPlacement::installGnomeExtension()
{
    const QDir extensionDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                            + QStringLiteral("/gnome-shell/extensions/") + kGnomeExtensionUuid);
    if (!extensionDir.mkpath(QStringLiteral("."))) {
        qCWarning(worktimeUtilities) << "failed to create" << extensionDir.path();
        return false;
    }

    bool changed = false;
    for (const ExtensionFile &file : kGnomeExtensionFiles) {
        QFile source(QString::fromLatin1(file.resourcePath));
        if (!source.open(QIODevice::ReadOnly)) {
            qCCritical(worktimeUtilities) << "failed to read bundled" << file.resourcePath;
            return false;
        }
        changed |= writeIfChanged(extensionDir.filePath(QString::fromLatin1(file.fileName)), source.readAll());
    }
    return changed;
}

bool PopupPlacement::callGnomeExtensionManager(const QString &method)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(s_gnomeShellExtensionsService,
                                                      s_gnomeShellExtensionsPath,
                                                      s_gnomeShellExtensionsService,
                                                      method);
    msg << kGnomeExtensionUuid.toString();
    const QDBusReply<bool> reply = QDBusConnection::sessionBus().call(msg);
    if (!reply.isValid()) {
        qCWarning(worktimeUtilities) << method << "D-Bus call failed:" << reply.error().message();
        return false;
    }
    return reply.value();
}

#endif

PopupPlacement::PopupPlacement(QObject *parent)
    : QObject(parent)
{}

void PopupPlacement::start()
{
#if defined(Q_OS_LINUX)
    if (!isWayland())
        return;

    if (desktopContains("gnome")) {
        // GNOME Shell only reloads an extension when it is disabled and enabled again,
        // so the new files take effect only after this cycle.
        if (const bool changed = installGnomeExtension(); changed)
            callGnomeExtensionManager(QStringLiteral("DisableExtension"));
        m_gnomeActive = callGnomeExtensionManager(QStringLiteral("EnableExtension"));
        if (!m_gnomeActive)
            qCInfo(worktimeUtilities) << "GNOME placement extension is not active yet; log out and back in to load it";
    } else if (desktopContains("kde")) {
        m_kwinActive = KWinScript::load(QString(kKWinPluginName),
                                        QStringLiteral(":/qt/qml/Worktime/popup-placement.js"),
                                        QStringLiteral("worktime-popup-placement.js"));
    }
#endif
}

void PopupPlacement::stop()
{
#if defined(Q_OS_LINUX)
    if (!m_kwinActive)
        return;

    KWinScript::unload(QString(kKWinPluginName));
    m_kwinActive = false;
#endif
}

void PopupPlacement::requestPlacement() const
{
#if defined(Q_OS_LINUX)
    if (!m_gnomeActive)
        return;

    QDBusMessage msg = QDBusMessage::createMethodCall(s_gnomePlacementService,
                                                      s_gnomePlacementPath,
                                                      s_gnomePlacementInterface,
                                                      QStringLiteral("PlaceNextPopup"));
    // Short timeout so a stalled shell can't freeze the tray click.
    const QDBusMessage reply = QDBusConnection::sessionBus().call(msg, QDBus::Block, 500);
    if (reply.type() != QDBusMessage::ReplyMessage)
        qCDebug(worktimeUtilities) << "PlaceNextPopup D-Bus call failed:" << reply.errorMessage();
#endif
}
