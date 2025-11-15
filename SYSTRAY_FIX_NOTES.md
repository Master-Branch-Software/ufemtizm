# System Tray and Autostart Fixes for Linux (KDE)

## Issues Fixed

1. **System tray icon not appearing after reboot**
2. **Application autostart timing issues on Linux/KDE**

## Root Causes

### System Tray Timing Issue
The application was checking `QSystemTrayIcon::isSystemTrayAvailable()` only once during initialization. On Linux, especially during system startup, the system tray service (StatusNotifier) may not be immediately available when the application launches. This caused the tray icon initialization to fail silently.

### Autostart Configuration Issue
The autostart desktop file was missing KDE-specific directives to ensure the application starts after the desktop environment is fully loaded, particularly after the system panel (which includes the system tray) is ready.

## Solutions Implemented

### 1. System Tray Retry Mechanism
Added a retry mechanism in `mainwindow.cpp` and `mainwindow.h`:

- New method: `initializeSystemTray()` - Attempts to initialize the system tray icon
- If the system tray is not available, it retries every second for up to 30 attempts
- Uses a `QTimer` to periodically check if the system tray becomes available
- Logs debug messages to help diagnose initialization issues

**Key changes:**
- Added `QTimer *trayInitTimer` and `int trayInitAttempts` member variables
- Implemented retry logic that waits up to 30 seconds for the system tray to become available
- Added debug logging: "System tray icon initialized successfully" or retry attempt messages

### 2. Updated Desktop File Configuration
Modified both the source desktop file and the runtime-generated autostart file:

**Added directives:**
- `StartupNotify=false` - Prevents startup notification that can interfere with tray-only apps
- `X-GNOME-Autostart-enabled=true` - Enables autostart for GNOME-based environments
- `X-KDE-autostart-after=panel` - Ensures the app starts after KDE's panel is loaded

**Files modified:**
- `unfuck-my-timezone-math.desktop` - Source desktop file
- `settingsdialog.cpp` - Runtime autostart file generation
- `~/.config/autostart/unfuck-my-timezone-math.desktop` - User's autostart configuration

## Testing

The application now successfully:
1. Initializes the system tray icon even when started early during boot
2. Retries tray initialization if the system tray isn't immediately available
3. Starts properly from autostart after system reboot
4. Logs initialization status for debugging

## Installation

To apply these fixes:

```bash
cd ~/Documents/UnfuckMyTimeZoneMath
./build_and_deploy.sh
sudo dpkg -i installers/UnfuckMyTimeZoneMath-1.0.3-Linux.deb
```

## Verification

After installation, check the log output:
```bash
UnfuckMyTimeZoneMath > /tmp/unfuck_debug.log 2>&1 &
sleep 3
cat /tmp/unfuck_debug.log
```

You should see: "System tray icon initialized successfully"

Or if the tray is delayed, you'll see retry attempts:
"System tray not available yet, retry attempt X of 30"

## Future Considerations

- The 30-second timeout should be sufficient for most systems
- If needed, the timeout can be adjusted in the `initializeSystemTray()` method
- Consider adding a user-facing notification if the system tray never becomes available
