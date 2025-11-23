# 🎉 Release 2025.0.0

## 📋 Overview
This major release introduces **comprehensive system tray functionality**, improved user experience, and several important bug fixes. The version numbering has been updated to a calendar-based scheme (2025.0.0).

---

## ✨ What's New

### 🔔 System Tray Integration
Full-featured system tray support is here!

- 🎯 **System tray icon** with context menu
- ⬇️ **Minimize to tray** - Hide window to tray instead of taskbar
- ❌ **Close to tray** - Keep app running when window closes
- 🚀 **Start minimized** - Launch directly to tray
- 🔄 **Run at login** - Cross-platform autostart (Linux, Windows, macOS)
- 🛡️ **Single instance** - Prevents multiple app instances
- 👆 **Double-click to toggle** - Quick window access from tray
- 📱 **Right-click menu** - Show/Hide, Recent Files, Settings, Quit
- ⏱️ **Smart retry** - 30-second timeout for reliable tray initialization

### ⚙️ Settings Dialog
- 🎛️ New Settings dialog accessible from toolbar and system tray
- 🎨 Configure all system tray preferences in one place
- 💾 Persistent settings across application restarts

### 🎨 UI Improvements
- 📊 **Better slider visualization** - Sliders fill from bottom to top
- 🖱️ **Fixed cursor scaling** - No more oversized cursors
- 🔘 **Settings button** added to toolbar

### 📋 Clipboard Enhancements
- ⏰ **Format-aware copying** - Respects each widget's time format preference
  - 24-hour mode: `14:30`
  - 12-hour mode: `2:30pm`
- 🌍 **Mixed format support** - Each timezone widget can use different formats

---

## 🐛 Bug Fixes
- ✅ Fixed slider value mapping after inverted appearance adjustment
- ✅ Fixed system tray icon not appearing after reboot on Linux
- ✅ Fixed autostart issues on Linux with KDE-specific directives
- ✅ Fixed install paths for consistency with DEB package structure
- ✅ Corrected time labels and slider-to-timestamp mapping
- ✅ Fixed window activation and show/hide operations

---

## 🖥️ Platform-Specific Improvements

### 🐧 Linux
- Added `X-KDE-autostart-after=panel` for proper startup timing
- Added `StartupNotify=false` to prevent tray interference
- Wrapper script for autostart ensures correct library paths
- `CMAKE_INSTALL_PREFIX` set to `/usr` for consistency

### 🍎 macOS
- System tray optimized for macOS behavior
- Info.plist configuration to control dock visibility
- DMG builds include correct version information

---

## 📚 Documentation
- 📝 Added `TESTING.md` with comprehensive manual test cases
- 📝 Added `SYSTRAY_FIX_NOTES.md` documenting system tray fixes
- 📝 Updated `README.md` with new features and clipboard behavior
- 🖼️ Added `InAction.png` screenshot

---

## 🔧 Technical Changes
- Refactored `CMakeLists.txt` for better platform handling
- Added `QLocalServer` for single instance enforcement
- Improved error handling and debug logging for system tray
- `Qt::Tool` flag for proper taskbar visibility control

---

## ⚠️ Breaking Changes
**None!** All changes are backward compatible with existing configuration files.

---

## 📦 Installation

### Linux (Debian/Ubuntu)
```bash
sudo dpkg -i UnfuckMyTimeZoneMath-2025.0.0-Linux.deb
```

### Windows
Run the installer executable

### macOS
Mount the DMG and drag to Applications

---

## 🔄 Upgrade Notes
- ✨ Autostart configurations automatically migrate to new wrapper script
- ✨ Existing settings files preserved and enhanced with new preferences

---

**Full Changelog**: [1.0.2...2025.0.0](../../compare/1.0.2...2025.0.0)
