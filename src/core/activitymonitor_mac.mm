#include "activitymonitor.hpp"

#import <ApplicationServices/ApplicationServices.h>

namespace {
ActivityMonitor *g_activityMonitor = nullptr;
CFMachPortRef g_eventTap = nullptr;

CGEventRef eventTapCallback(CGEventTapProxy /*proxy*/, CGEventType type, CGEventRef event, void * /*userInfo*/)
{
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        if (g_eventTap)
            CGEventTapEnable(g_eventTap, true);
        return event;
    }

    if (g_activityMonitor)
        emit g_activityMonitor->activityDetected();

    return event;
}
} // namespace

void ActivityMonitor::start()
{
    if (m_running)
        return;

    const CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp)
                             | CGEventMaskBit(kCGEventFlagsChanged) | CGEventMaskBit(kCGEventLeftMouseDown)
                             | CGEventMaskBit(kCGEventLeftMouseUp) | CGEventMaskBit(kCGEventRightMouseDown)
                             | CGEventMaskBit(kCGEventRightMouseUp) | CGEventMaskBit(kCGEventMouseMoved)
                             | CGEventMaskBit(kCGEventLeftMouseDragged) | CGEventMaskBit(kCGEventRightMouseDragged)
                             | CGEventMaskBit(kCGEventScrollWheel);

    CFMachPortRef eventTap = CGEventTapCreate(kCGSessionEventTap,
                                              kCGHeadInsertEventTap,
                                              kCGEventTapOptionListenOnly,
                                              mask,
                                              eventTapCallback,
                                              nullptr);
    if (!eventTap)
        return; // Likely missing Accessibility/Input Monitoring permission.

    g_activityMonitor = this;
    g_eventTap = eventTap;
    m_eventTap = eventTap;

    CFRunLoopSourceRef runLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    m_runLoopSource = runLoopSource;
    CFRunLoopAddSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);

    m_running = true;
}

void ActivityMonitor::stop()
{
    if (!m_running)
        return;

    if (m_runLoopSource) {
        auto *runLoopSource = static_cast<CFRunLoopSourceRef>(m_runLoopSource);
        CFRunLoopRemoveSource(CFRunLoopGetMain(), runLoopSource, kCFRunLoopCommonModes);
        CFRelease(runLoopSource);
        m_runLoopSource = nullptr;
    }

    if (m_eventTap) {
        auto *eventTap = static_cast<CFMachPortRef>(m_eventTap);
        CGEventTapEnable(eventTap, false);
        CFRelease(eventTap);
        m_eventTap = nullptr;
    }

    g_eventTap = nullptr;
    g_activityMonitor = nullptr;
    m_running = false;
}
