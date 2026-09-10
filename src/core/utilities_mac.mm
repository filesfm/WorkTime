// macOS implementation of Utilities::focusedApplicationName(),
// activeWindowTitle(), and activeWindowExecutablePath(). Split into its own
// Objective-C++ translation unit because NSWorkspace has no C++ API.

#include "utilities.hpp"

#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#include <ServiceManagement/ServiceManagement.h>

namespace {

NSRunningApplication *frontmostApplication()
{
    return [[NSWorkspace sharedWorkspace] frontmostApplication];
}

} // namespace

QString Utilities::focusedApplicationName()
{
    NSRunningApplication *app = frontmostApplication();
    if (!app)
        return QString();

    return QString::fromNSString(app.localizedName);
}

QString Utilities::activeWindowExecutablePath()
{
    NSRunningApplication *app = frontmostApplication();
    if (!app)
        return QString();

    return QString::fromNSString(app.executableURL.path);
}

QString Utilities::activeWindowTitle()
{
    // Requires the Accessibility permission (System Settings > Privacy &
    // Security > Accessibility) to be granted to this app; without it,
    // AXUIElementCopyAttributeValue fails with kAXErrorAPIDisabled and this
    // falls back to an empty string, the same "unavailable" contract used
    // elsewhere (e.g. Wayland on Linux).
    NSRunningApplication *app = frontmostApplication();
    if (!app)
        return QString();

    AXUIElementRef appElement = AXUIElementCreateApplication(app.processIdentifier);
    if (!appElement)
        return QString();

    CFTypeRef windowValue = nullptr;
    const AXError windowError = AXUIElementCopyAttributeValue(appElement, kAXFocusedWindowAttribute, &windowValue);

    QString title;
    if (windowError == kAXErrorSuccess && windowValue) {
        const auto window = static_cast<AXUIElementRef>(const_cast<void *>(windowValue));

        CFTypeRef titleValue = nullptr;
        const AXError titleError = AXUIElementCopyAttributeValue(window, kAXTitleAttribute, &titleValue);
        if (titleError == kAXErrorSuccess && titleValue) {
            title = QString::fromCFString(static_cast<CFStringRef>(titleValue));
            CFRelease(titleValue);
        }
        CFRelease(windowValue);
    }

    CFRelease(appElement);
    return title;
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
