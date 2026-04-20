#ifndef MACOS_HELPER_HPP
#define MACOS_HELPER_HPP

// macOS-only helpers for switching the running app between the "accessory"
// (background, tray-only: no Dock icon, no global menu bar) and "regular"
// (foreground: Dock icon and global menu bar) activation policies at runtime.
//
// The app is packaged with LSUIElement=true in Info.plist so it launches
// quietly as an accessory. Call setMacosActivationPolicyRegular() when the
// main window becomes visible to acquire the menu bar, and
// setMacosActivationPolicyAccessory() when it is hidden back to the tray.

void setMacosActivationPolicyRegular();

void setMacosActivationPolicyAccessory();

#endif // MACOS_HELPER_HPP
