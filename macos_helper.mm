#include "macos_helper.hpp"

#import <AppKit/AppKit.h>

void setMacosActivationPolicyRegular(){
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Without activating, promoting from Accessory to Regular does not always
    // bring the new global menu bar to the front until the user clicks the
    // window.
    [NSApp activateIgnoringOtherApps:YES];
}

void setMacosActivationPolicyAccessory(){
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}
