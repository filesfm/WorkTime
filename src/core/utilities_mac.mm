#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <ServiceManagement/ServiceManagement.h>

QString Utilities::focusedApplicationName()
{
    NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
    pid_t pid = [app processIdentifier];

    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    Boolean appHasPermission = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);

    if (!appHasPermission) {
        return QString();
    }

    AXUIElementRef appElem = AXUIElementCreateApplication(pid);
    if (!appElem) {
        return QString();
    }

    AXUIElementRef window = NULL;
    if (AXUIElementCopyAttributeValue(appElem, kAXFocusedWindowAttribute, (CFTypeRef *)&window) != kAXErrorSuccess) {
        CFRelease(appElem);
        return QString();
    }

    CFStringRef title = NULL;
    AXError result = AXUIElementCopyAttributeValue(window, kAXTitleAttribute, (CFTypeRef *)&title);

    CFRelease(window);
    CFRelease(appElem);

    if (result != kAXErrorSuccess) {
        return QString();
    }

    QString titleStr = QString::fromCFString(title);
    CFRelease(title);

    return titleStr;
}

void Utilities::autostart(bool autostart)
{
    SMAppService *service = [SMAppService mainAppService];
    NSError *error = nil;

    if (autostart) {
        [service registerAndReturnError:&error];
    } else {
        [service unregisterAndReturnError:&error];
    }
}
