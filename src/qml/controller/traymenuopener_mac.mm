#include "traymenuopener.hpp"

#import <Foundation/Foundation.h>

void TrayMenuOpener::open(const QJSValue &openMenu)
{
    QJSValue callback = openMenu;
    [[NSRunLoop mainRunLoop] performInModes:@[NSDefaultRunLoopMode]
                                      block:^{
                                        QJSValue copy = callback;
                                        copy.call();
                                      }];
    CFRunLoopWakeUp(CFRunLoopGetMain());
}
