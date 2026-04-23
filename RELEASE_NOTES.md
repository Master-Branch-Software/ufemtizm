# 🎉 Release 2025.1.0

## 📋 Overview
This release focuses on **UI/UX polish**, **visual refinements**, and **platform-specific fixes**. Significant improvements include refined sky color transitions, improved code quality with comprehensive style standardization, and better cross-platform system tray behavior.

---

## ✨ What's New

### 🎨 Visual Enhancements
- **Refined sky color transitions** - Smoother, more natural color progression throughout the day
  - Extended night mode (red text) from 8 PM to 7 AM for better visibility
  - Removed warm dawn colors for cleaner transition from night to morning blue
  - Improved color gradients for noon peak and sunset periods
  - Button colors now fade more naturally with sky colors (20% darker)
- **Reduced sky color opacity** - 20% reduction for more subtle, less distracting background colors
- **Right-aligned slider time labels** - Cleaner, more professional appearance
- **3D appearance improvements** - Enhanced subtle drop shadows for better depth perception

### 🖌️ UI/UX Improvements
- **Compact toolbar interface** - Complete redesign with intuitive icon-based controls:
  - Single toggle button for 12/24-hour format switching
  - Globe icon (🌍) for quick timezone selection via dropdown
  - X button integrated into toolbar for cleaner widget removal
- **Double-click name editing** - Click widget names to edit, click outside to save
- **Improved interaction feedback** - Better visual cues for interactive elements
- **Timezone display** - Timezone name now shown below time for better context

### 🐛 Platform-Specific Fixes
- **Windows**: Fixed systray double-click not responding
  - Changed to DoubleClick events only (previously used Trigger)
- **Linux**: Fixed window not appearing in Alt+Tab task switcher
  - Removed Qt::Tool flag on Linux (kept on Windows for proper behavior)
- **Linux/KDE**: Fixed black window rendering on startup
  - Added proper widget attributes for background rendering
  - Added processEvents() calls after show operations
  - Known issue: First systray double-click may still flash black (workaround: use right-click menu)
- **Cross-platform**: Improved tray icon activation handling for platform-specific behavior

### 🛠️ Code Quality
- **Complete C++ code reformatting** to match project style guidelines:
  - Renamed all `.h` files to `.hpp` extensions
  - Updated include guards to `FILENAME_HPP` format
  - Standardized brace placement (same line for functions, classes, control structures)
  - Fixed constructor initializer list formatting
  - Updated all includes and build files across all platforms

---

## 📚 Documentation
- 📝 Added comprehensive `PLATFORM_FIXES.md` documenting all platform-specific issues and solutions
- 📝 Updated `README.md` with latest UI features and troubleshooting
- 📝 Enhanced sky color theme documentation

---

## 🔧 Technical Changes
- Standardized all C++ code to project conventions
- Improved platform detection and conditional compilation
- Enhanced CMakeLists.txt for better cross-platform support
- Better separation of platform-specific UI behaviors

---

## ⚠️ Known Issues
- **Linux/KDE**: First double-click from systray may show black window briefly
  - **Workaround**: Use right-click menu "Show Window" on first show
  - Subsequent shows work correctly

---

## 📦 Installation

### Linux (Debian/Ubuntu)
```bash
sudo dpkg -i Ufemtizm-2025.1.0-Linux.deb
```

### Windows
Run the installer executable

### macOS
Mount the DMG and drag to Applications

---

## 🔄 Upgrade Notes
- ✨ All changes are backward compatible
- ✨ Existing settings preserved
- ✨ Sky color preferences automatically saved

---

**Full Changelog**: [2025.0.0...2025.1.0](../../compare/2025.0.0...2025.1.0)

---

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
sudo dpkg -i Ufemtizm-2025.0.0-Linux.deb
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
