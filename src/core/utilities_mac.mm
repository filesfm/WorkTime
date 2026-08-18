// macOS implementation of Utilities::focusedApplicationName(). Split into
// its own Objective-C++ translation unit because NSWorkspace has no C++ API.

#include "utilities.hpp"

#import <AppKit/AppKit.h>

QString Utilities::focusedApplicationName()
{
    NSRunningApplication *app = [[NSWorkspace sharedWorkspace] frontmostApplication];
    if (!app)
        return QString();

    return QString::fromNSString(app.localizedName);
}
