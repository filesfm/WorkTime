#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
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
    qCWarning(worktimeUtilities) << "showNotification not yet implemented on macOS:" << title << message;
}
