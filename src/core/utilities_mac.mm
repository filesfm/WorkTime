#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <UserNotifications/UserNotifications.h>
#include <ServiceManagement/ServiceManagement.h>

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

void Utilities::autostart(bool autostart)
{
    qCInfo(worktimeUtilities) << (autostart ? "enabling" : "disabling") << "autostart";

    SMAppService *service = [SMAppService mainAppService];
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
                                qCWarning(worktimeUtilities)
                                << "requestAuthorization failed:"
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
