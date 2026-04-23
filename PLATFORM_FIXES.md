# Platform-Specific Fixes

## Issues Fixed (December 2025)

### Windows: System Tray Double-Click Not Working

**Problem:**
Double-clicking the system tray icon on Windows did not show/hide the window.

**Root Cause:**
On Windows, Qt's `QSystemTrayIcon::Trigger` event is only emitted for single left-clicks, NOT for double-clicks. The code was checking for both `Trigger` OR `DoubleClick`, but on Windows only `DoubleClick` is emitted when the user double-clicks the icon.

**Solution:**
Implemented platform-specific activation handling in `mainwindow.cpp`:
- **Windows**: Only respond to `DoubleClick` events
- **Linux/macOS**: Respond to both `Trigger` (single click) and `DoubleClick` events

**Code Changes:**
```cpp
void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
#ifdef Q_OS_WIN
    // On Windows, only DoubleClick is emitted for double-click
    if (reason == QSystemTrayIcon::DoubleClick)
    {
        toggleWindowVisibility();
    }
#else
    // On Linux/macOS, Trigger is emitted for single click
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
    {
        toggleWindowVisibility();
    }
#endif
}
```

---

### Linux: Application Not Visible in Alt+Tab Task Switcher

**Problem:**
When the application window was visible on Linux, it did not appear in the Alt+Tab task switcher, making it difficult to switch back to the window.

**Root Cause:**
The `Qt::Tool` window flag was being set on all non-macOS platforms. This flag is designed for tool windows (like floating toolbars) and specifically excludes windows from the task switcher. While this is appropriate on Windows where tray-only apps should be hidden, on Linux users expect visible windows to appear in Alt+Tab.

**Solution:**
Changed the window flags to only apply `Qt::Tool` on Windows:
- **Windows**: Apply `Qt::Tool` flag to hide from taskbar (tray-only app behavior)
- **Linux**: Do NOT apply `Qt::Tool` flag, allowing window to appear in task switcher when visible
- **macOS**: Uses `LSUIElement` in `Info.plist` instead of Qt flags

**Code Changes:**
```cpp
// Hide from taskbar on Windows only (macOS uses LSUIElement in Info.plist)
// On Linux, we want the window to appear in Alt+Tab when visible
#ifdef Q_OS_WIN
    setWindowFlags(windowFlags() | Qt::Tool);
#endif
```

---

### Linux: Black Window on Startup and When Reshowing

**Problem:**
When launching the application on Linux (especially KDE/Plasma), the main window would appear completely black for a moment before rendering properly. This also occurred when showing the window after it had been hidden (e.g., clicking the systray icon or selecting "Show Window" from the menu).

**Root Cause:**
The window was being shown before the compositor had fully initialized the window's paint events. On KDE Plasma, the window manager needs explicit widget attributes and must process pending events after showing the window to properly composite it.

**Solution:**
Implemented Linux-specific window initialization and display handling:
1. Set `Qt::WA_OpaquePaintEvent` and `Qt::WA_NoSystemBackground` to false to ensure proper background rendering
2. Changed the initial window show sequence to: show → activate → raise → process events
3. Added `QApplication::processEvents()` after every window show operation on Linux
4. This applies to: initial startup, systray toggle, opening recent files, and single-instance activation

**Code Changes in `mainwindow.cpp`:**
```cpp
// Fix black window on Linux by ensuring proper paint events
#ifdef Q_OS_LINUX
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
#endif
```

**Code Changes in `main.cpp`:**
```cpp
#ifdef Q_OS_LINUX
    // On Linux, ensure window is fully initialized before showing
    // This prevents the black window issue on KDE/Plasma
    window.show();
    window.activateWindow();
    window.raise();
    app.processEvents();
#else
    // Process pending events to ensure window is properly initialized before showing
    app.processEvents();
    window.show();
    window.repaint();
#endif
```

**Code Changes in `mainwindow.cpp` (all window show locations):**
```cpp
// In toggleWindowVisibility(), openRecentFileFromTray(), and handleNewConnection()
setWindowState(Qt::WindowNoState);
showNormal();
raise();
activateWindow();

#ifdef Q_OS_LINUX
    // Process events to prevent black window on KDE/Plasma
    QApplication::processEvents();
#endif
```

---

## Known Issues

### Linux: Black Window on First Double-Click from Systray

**Issue:**
On Linux (KDE/Plasma), when the application starts minimized to the systray and you double-click the systray icon for the **first time** to show the window, it may still appear black momentarily before rendering properly. Subsequent show/hide operations work correctly.

**Workaround:**
Instead of double-clicking the systray icon on first show, right-click and select "Show Window" from the context menu. This consistently shows the window without the black flash.

**Why This Happens:**
The KDE Plasma compositor requires additional initialization time when showing a window for the very first time after application startup. The `processEvents()` call helps but may not be sufficient for the initial compositor handshake in all cases, especially when triggered via double-click activation.

**Future Fix:**
This could potentially be resolved by:
1. Adding a longer delay or multiple `processEvents()` calls on first show
2. Pre-initializing the window visibility state during startup (showing and immediately hiding)
3. Using a platform-specific compositor sync API if available

However, these solutions add complexity and potential side effects, so the current workaround (use menu on first show) is recommended.

---

## Platform Behavior Summary

| Platform | Systray Click Behavior | Task Switcher Behavior |
|----------|------------------------|------------------------|
| **Windows** | Double-click to show/hide | Hidden from taskbar (tray-only) |
| **Linux** | Single-click to show/hide | Visible in Alt+Tab when window shown |
| **macOS** | Single-click to show/hide | Hidden from dock (LSUIElement) |

---

## Testing

### Windows Testing
1. Build and install the application
2. Double-click the system tray icon
3. Verify the window shows/hides correctly
4. Verify the window does not appear in the taskbar

### Linux Testing
1. Build and install: `sudo dpkg -i installers/Ufemtizm-2025.0.0-Linux.deb`
2. Launch the application
3. Click the system tray icon to show the window
4. Press Alt+Tab and verify the application appears in the task switcher
5. Switch to another application and back using Alt+Tab
6. Click the system tray icon again to hide the window

---

## Installation

To apply these fixes:

```bash
cd ~/Documents/Ufemtizm
./build_and_deploy.sh
sudo dpkg -i installers/Ufemtizm-2025.0.0-Linux.deb
```

For Windows, build the Windows installer and deploy accordingly.
