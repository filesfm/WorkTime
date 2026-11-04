#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <ServiceManagement/ServiceManagement.h>
#import <UserNotifications/UserNotifications.h>
#include <xpc/xpc.h>

QString Utilities::focusedWindowTitle()
{
    NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
    pid_t pid = [app processIdentifier];

    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    Boolean appHasPermission = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);

    if (!appHasPermission) {
        qCWarning(worktimeUtilities) << "accessibility permission not granted";
        return QString();
    }

    AXUIElementRef appElem = AXUIElementCreateApplication(pid);
    if (!appElem) {
        qCWarning(worktimeUtilities) << "failed to create AXUIElement for pid" << pid;
        return QString();
    }

    AXUIElementRef window = NULL;
    if (AXUIElementCopyAttributeValue(appElem, kAXFocusedWindowAttribute, (CFTypeRef *)&window) != kAXErrorSuccess) {
        qCWarning(worktimeUtilities) << "failed to get focused window attribute";
        CFRelease(appElem);
        return QString();
    }

    CFStringRef title = NULL;
    AXError result = AXUIElementCopyAttributeValue(window, kAXTitleAttribute, (CFTypeRef *)&title);

    CFRelease(window);
    CFRelease(appElem);

    if (result != kAXErrorSuccess) {
        qCWarning(worktimeUtilities) << "failed to get window title attribute";
        return QString();
    }

    QString titleStr = QString::fromCFString(title);
    CFRelease(title);

    return titleStr;
}

#if !defined(BUILD_WITHOUT_AUTOSTART)

void *Utilities::s_autostartAppService = nullptr;

void Utilities::autostart(bool autostart)
{
    qCInfo(worktimeUtilities) << (autostart ? "enabling" : "disabling") << "autostart";

    SMAppService *service = s_autostartAppService ? static_cast<SMAppService *>(s_autostartAppService)
                                                  : [SMAppService mainAppService];
    NSError *error = nil;

    if (autostart) {
        [service registerAndReturnError:&error];
    } else {
        [service unregisterAndReturnError:&error];
    }

    if (error) {
        qCWarning(worktimeUtilities) << "SMAppService call failed:"
                                     << QString::fromNSString(error.localizedDescription);
    }
}

bool Utilities::isAutostartEnabled()
{
    SMAppService *service = s_autostartAppService ? static_cast<SMAppService *>(s_autostartAppService)
                                                  : [SMAppService mainAppService];
    const SMAppServiceStatus status = service.status;

    return status == SMAppServiceStatusEnabled || status == SMAppServiceStatusRequiresApproval;
}

#endif // !defined(BUILD_WITHOUT_AUTOSTART)

const char *Utilities::s_clockSyncMachService = "com.apple.timed.xpc";

bool Utilities::isClockSyncEnabled()
{
    xpc_connection_t connection = xpc_connection_create_mach_service(s_clockSyncMachService, nullptr, 0);
    xpc_connection_set_event_handler(connection,
                                     ^(xpc_object_t){
                                     });
    xpc_connection_resume(connection);

    xpc_object_t request = xpc_dictionary_create(nullptr, nullptr, 0);
    xpc_dictionary_set_string(request, "TMCommand", "TMIsAutomaticTimeEnabled");
    xpc_object_t reply = xpc_connection_send_message_with_reply_sync(connection, request);
    xpc_release(request);
    xpc_connection_cancel(connection);
    xpc_release(connection);

    bool enabled = true;
    if (xpc_get_type(reply) != XPC_TYPE_DICTIONARY) {
        qCWarning(worktimeUtilities) << "timed did not answer, assuming clock sync is enabled";
    } else if (xpc_dictionary_get_value(reply, "TMAutomaticTimeEnabled") == nullptr) {
        qCWarning(worktimeUtilities) << "timed reply has no TMAutomaticTimeEnabled, assuming clock sync is enabled";
    } else {
        enabled = xpc_dictionary_get_bool(reply, "TMAutomaticTimeEnabled");
    }
    xpc_release(reply);
    return enabled;
}

void Utilities::showNotification(const QString &title, const QString &message)
{
    // UNUserNotificationCenter raises an exception when the app has no bundle identifier, which
    // happens when the executable is run directly instead of through WorkTime.app.
    if (![NSBundle mainBundle].bundleIdentifier) {
        qCWarning(worktimeUtilities) << "no bundle identifier, cannot notify:" << title << message;
        return;
    }

    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center requestAuthorizationWithOptions:UNAuthorizationOptionAlert | UNAuthorizationOptionSound
                          completionHandler:^(BOOL granted, NSError *error) {
                            if (error) {
                                qCWarning(worktimeUtilities) << "requestAuthorization failed:"
                                                             << QString::fromNSString(error.localizedDescription);
                            } else if (!granted) {
                                qCWarning(worktimeUtilities) << "notification permission not granted";
                            }
                          }];

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();
    content.body = message.toNSString();
    content.sound = [UNNotificationSound defaultSound];

    // A nil trigger delivers immediately; the identifier is unique so notifications never replace
    // one another.
    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:[[NSUUID UUID] UUIDString]
                                                                          content:content
                                                                          trigger:nil];
    [content release];

    [center addNotificationRequest:request
             withCompletionHandler:^(NSError *error) {
               if (error) {
                   qCWarning(worktimeUtilities)
                       << "addNotificationRequest failed:" << QString::fromNSString(error.localizedDescription);
               }
             }];
}
