#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <ServiceManagement/ServiceManagement.h>
#import <UserNotifications/UserNotifications.h>

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

bool Utilities::isClockSyncEnabled()
{
    NSTask *task = [[NSTask alloc] init];
    task.launchPath = @"/usr/sbin/systemsetup";
    task.arguments = @[@"-getusingnetworktime"];

    NSPipe *outputPipe = [NSPipe pipe];
    task.standardOutput = outputPipe;
    task.standardError = [NSPipe pipe]; // discard stderr (e.g. permission-denied chatter)

    NSError *error = nil;
    BOOL launched = [task launchAndReturnError:&error];
    if (!launched) {
        qCWarning(worktimeUtilities) << "systemsetup -getusingnetworktime failed to launch:"
                                     << QString::fromNSString(error.localizedDescription);
        [task release];
        return true;
    }

    NSData *outputData = [[outputPipe fileHandleForReading] readDataToEndOfFile];
    [task waitUntilExit];
    const int status = task.terminationStatus;
    [task release];

    if (status != 0) {
        qCWarning(worktimeUtilities) << "systemsetup -getusingnetworktime exited with status" << status
                                     << "(may need elevated privileges), assuming clock sync is enabled";
        return true;
    }

    NSString *outputString = [[NSString alloc] initWithData:outputData encoding:NSUTF8StringEncoding];
    const QString output = QString::fromNSString(outputString);
    [outputString release];

    if (output.contains(QStringLiteral("Network Time: On"), Qt::CaseInsensitive))
        return true;
    if (output.contains(QStringLiteral("Network Time: Off"), Qt::CaseInsensitive))
        return false;

    qCWarning(worktimeUtilities) << "could not parse systemsetup output, assuming clock sync is enabled:" << output;
    return true;
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
