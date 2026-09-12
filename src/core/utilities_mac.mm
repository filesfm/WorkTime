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
