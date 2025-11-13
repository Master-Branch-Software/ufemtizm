# Building with MinGW

This document explains how to build UnfuckMyTimeZoneMath using the MinGW compiler for Windows.

## Overview

MinGW (Minimalist GNU for Windows) is a minimalist development environment for native Microsoft Windows applications. The project is configured to prefer MinGW over MSVC when building for Windows.

## Building on Windows with MinGW

### Prerequisites

1. **MinGW Compiler**: Install MinGW-w64
   - Download from [mingw-w64.org](https://www.mingw-w64.org/)
   - Or install via MSYS2: `pacman -S mingw-w64-x86_64-gcc`

2. **Qt for MinGW**: Install Qt with MinGW support
   - Download from [qt.io](https://www.qt.io/download)
   - Select the MinGW kit during installation (e.g., `mingw81_64`, `mingw_64`)

3. **CMake**: Version 3.16 or later

### Automated Build

The `build_and_deploy.sh` script automatically detects and prefers MinGW:

```bash
./build_and_deploy.sh
```

The script searches for Qt installations in this order:
1. `$HOME/Qt/<version>/mingw*` (MinGW first)
2. `$HOME/Qt/<version>/msvc*` (MSVC fallback)
3. `C:/Qt/<version>/mingw*` (MinGW first)
4. `C:/Qt/<version>/msvc*` (MSVC fallback)

### Manual Build with MinGW

```bash
mkdir build
cd build

# Set Qt path to MinGW installation
cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_xx ^
      -G "MinGW Makefiles" ^
      -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Deploy Qt dependencies
C:/Qt/6.x.x/mingw_xx/bin/windeployqt.exe --release UnfuckMyTimeZoneMath.exe
```

## Cross-Compiling from Linux

You can cross-compile for Windows from Linux using MinGW:

### Prerequisites

Install MinGW cross-compiler on Linux:

```bash
# Ubuntu/Debian
sudo apt install mingw-w64

# Fedora
sudo dnf install mingw64-gcc mingw64-gcc-c++

# Arch
sudo pacman -S mingw-w64-gcc
```

### Using the Toolchain File

A MinGW toolchain file (`mingw-toolchain.cmake`) is provided for cross-compilation:

```bash
mkdir build-windows
cd build-windows

# Configure with MinGW toolchain
cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-toolchain.cmake \
      -DCMAKE_PREFIX_PATH=/path/to/qt/for/mingw \
      -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release
```

### Installing Qt for MinGW on Linux

For cross-compilation, install Qt built for MinGW:

```bash
# Option 1: Use MXE (M cross environment)
git clone https://github.com/mxe/mxe.git
cd mxe
make qt6

# Option 2: Download from Qt online installer
# Select Qt for MinGW during installation
```

## Verifying MinGW Build

To verify the executable was built with MinGW:

```bash
# On Windows (check dependencies)
dumpbin /dependents UnfuckMyTimeZoneMath.exe

# On Linux (after cross-compile)
x86_64-w64-mingw32-objdump -p UnfuckMyTimeZoneMath.exe | grep "DLL Name"
```

You should see MinGW runtime libraries like:
- `libgcc_s_seh-1.dll`
- `libstdc++-6.dll`
- `libwinpthread-1.dll`

## Troubleshooting

### MinGW Not Found

**Error**: CMake cannot find the compiler

**Solution**: Ensure MinGW bin directory is in PATH:
```bash
# Windows
set PATH=C:\mingw64\bin;%PATH%

# Or specify compiler explicitly
cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ ..
```

### Qt MinGW Kit Not Found

**Error**: Cannot find Qt for MinGW

**Solution**: Download and install Qt with MinGW support, or specify the path:
```bash
cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/mingw_xx ..
```

### Missing DLL Files

**Error**: Application won't start due to missing DLLs

**Solution**: Run windeployqt or manually copy MinGW runtime DLLs:
```bash
# Use windeployqt (recommended)
C:/Qt/6.x.x/mingw_xx/bin/windeployqt.exe --release UnfuckMyTimeZoneMath.exe

# Or copy MinGW DLLs manually
copy C:\mingw64\bin\libgcc_s_seh-1.dll .
copy C:\mingw64\bin\libstdc++-6.dll .
copy C:\mingw64\bin\libwinpthread-1.dll .
```

### Static Linking

To create a fully static executable (no external DLLs):

1. Edit `mingw-toolchain.cmake` (already configured):
   ```cmake
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
   ```

2. Build Qt statically (advanced):
   ```bash
   ./configure -static -prefix /opt/qt6-static -opensource -confirm-license
   cmake --build . --parallel
   cmake --install .
   ```

## Comparison: MinGW vs MSVC

| Feature | MinGW | MSVC |
|---------|-------|------|
| License | Free, open source | Proprietary (free community edition) |
| Size | Smaller (~100MB) | Larger (~several GB) |
| C++ Standard | Good support | Excellent support |
| Windows API | Full support | Native support |
| Debugging | GDB | Visual Studio debugger |
| Cross-compile | Yes (from Linux) | No |
| Performance | Good | Slightly better |

For this project, MinGW is preferred for:
- Smaller footprint
- Cross-platform build consistency
- Free and open-source toolchain
- Cross-compilation capability
