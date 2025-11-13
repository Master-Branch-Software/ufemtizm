# Building and Deploying UnfuckMyTimeZoneMath

This guide explains how to build and create installers for UnfuckMyTimeZoneMath across all supported platforms: macOS, Linux, and Windows.

## Quick Start

The easiest way to build and deploy is using the included build script:

```bash
./build_and_deploy.sh
```

This script will:
- Detect your operating system automatically
- Find your Qt installation
- Build the application
- Deploy Qt dependencies
- Create a platform-specific installer
- Copy the installer to the `installers/` directory

## Platform-Specific Requirements

### macOS

**Requirements:**
- CMake 3.16+
- Qt6 (via Qt Creator or Homebrew)
- Xcode Command Line Tools

**Output:**
- `build/UnfuckMyTimeZoneMath.app` - Application bundle
- `installers/unfuck-my-timezone-math-1.0.0-Darwin.dmg` - DMG installer

**Installation:**
```bash
# Via Homebrew (optional)
brew install qt@6 cmake

# Or download Qt from https://www.qt.io/download
```

### Linux

**Requirements:**
- CMake 3.16+
- Qt6 development packages
- GCC or Clang
- dpkg tools (for .deb packaging)

**Output:**
- `build/UnfuckMyTimeZoneMath` - Executable
- `installers/unfuck-my-timezone-math-1.0.0-Linux.deb` - DEB package

**Installation (Ubuntu/Debian):**
```bash
sudo apt-get install cmake qt6-base-dev build-essential
```

**Installation (Fedora):**
```bash
sudo dnf install cmake qt6-qtbase-devel gcc-c++
```

**Optional - linuxdeployqt:**
```bash
# Download from https://github.com/probonopd/linuxdeployqt/releases
wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod +x linuxdeployqt-continuous-x86_64.AppImage
sudo mv linuxdeployqt-continuous-x86_64.AppImage /usr/local/bin/linuxdeployqt
```

### Windows

**Requirements:**
- CMake 3.16+
- Qt6 (via Qt Creator)
- Visual Studio 2019+ or MinGW
- NSIS (optional, for installer creation)

**Output:**
- `build/UnfuckMyTimeZoneMath.exe` - Executable
- `installers/unfuck-my-timezone-math-1.0.0-win64.exe` - NSIS installer (if NSIS is installed)
- `installers/unfuck-my-timezone-math-1.0.0-win64.zip` - ZIP package (fallback)

**Installation:**
1. Download and install Qt from https://www.qt.io/download
2. Install CMake from https://cmake.org/download/
3. Install Visual Studio or MinGW
4. (Optional) Install NSIS from https://nsis.sourceforge.io/

## Manual Build Process

If you prefer to build manually without the script:

### Step 1: Configure

```bash
mkdir build
cd build

# Set Qt path based on your platform
# macOS: ~/Qt/6.x.x/macos
# Linux: ~/Qt/6.x.x/gcc_64
# Windows: C:/Qt/6.x.x/mingw_xx or C:/Qt/6.x.x/msvc2019_64

cmake -DCMAKE_PREFIX_PATH=/path/to/qt -DCMAKE_BUILD_TYPE=Release ..
```

### Step 2: Build

```bash
cmake --build . --config Release
```

### Step 3: Deploy Qt Dependencies

**macOS:**
```bash
/path/to/qt/bin/macdeployqt UnfuckMyTimeZoneMath.app
```

**Linux:**
```bash
linuxdeployqt UnfuckMyTimeZoneMath
```

**Windows:**
```bash
/path/to/qt/bin/windeployqt.exe --release UnfuckMyTimeZoneMath.exe
```

### Step 4: Create Installer

```bash
# macOS - Creates DMG
cpack -G DragNDrop

# Linux - Creates DEB
cpack -G DEB

# Windows - Creates NSIS installer or ZIP
cpack -G NSIS
# or
cpack -G ZIP
```

### Step 5: Copy to Installers Directory

```bash
# From build directory, copy installers
cd ..
mkdir -p installers
cp build/*.dmg installers/  # macOS
cp build/*.deb installers/  # Linux
cp build/*.exe installers/  # Windows
cp build/*.zip installers/  # Windows ZIP
```

## Environment Variables

You can override the automatic Qt detection by setting:

```bash
export QT_PATH=/path/to/your/qt/installation
./build_and_deploy.sh
```

## Troubleshooting

### Qt not found

If the script cannot find Qt:
1. Install Qt from https://www.qt.io/download
2. Set `QT_PATH` environment variable
3. Or edit the script to specify your Qt path

### Build fails on macOS

Ensure Xcode Command Line Tools are installed:
```bash
xcode-select --install
```

### Build fails on Linux

Make sure all Qt6 development packages are installed:
```bash
sudo apt-get install qt6-base-dev qt6-base-dev-tools
```

### Windows deployment issues

1. Ensure you're using the correct Qt kit (MinGW or MSVC) matching your compiler
2. Add Qt bin directory to PATH before running cmake
3. Use the Qt-provided command prompt

## CI/CD Integration

The `build_and_deploy.sh` script is designed to work in CI/CD pipelines:

```yaml
# GitHub Actions example
- name: Build and Deploy
  run: ./build_and_deploy.sh
  env:
    QT_PATH: ${{ env.Qt6_DIR }}
```

## Distribution

After building, installers are automatically copied to the `installers/` directory:

- **macOS**: Distribute the `.dmg` file from `installers/`. Users drag the app to Applications folder.
- **Linux**: Distribute the `.deb` package from `installers/`. Users install with `sudo dpkg -i package.deb`.
- **Windows**: Distribute the `.exe` installer or `.zip` package from `installers/`.

All installers are self-contained with Qt dependencies bundled.

## Repository Structure

The `installers/` directory contains pre-built installers for all platforms. These can be:
- Downloaded directly from the repository
- Distributed to end users
- Uploaded to release pages
- Shared via cloud storage or internal networks
