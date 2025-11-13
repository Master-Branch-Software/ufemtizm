#!/bin/bash

set -e

APP_NAME="UnfuckMyTimeZoneMath"
BUILD_DIR="build"
INSTALLER_DIR="installers"

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
        echo "windows"
    else
        echo "unknown"
    fi
}

find_qt_path() {
    local os=$1
    
    if [[ "$os" == "macos" ]]; then
        if [[ -d "$HOME/Qt" ]]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [[ -n "$QT_VERSION" && -d "$HOME/Qt/$QT_VERSION/macos" ]]; then
                echo "$HOME/Qt/$QT_VERSION/macos"
                return
            fi
        fi
        if command -v brew &> /dev/null; then
            local brew_qt=$(brew --prefix qt@6 2>/dev/null)
            if [[ -n "$brew_qt" ]]; then
                echo "$brew_qt"
                return
            fi
        fi
    elif [[ "$os" == "linux" ]]; then
        if [[ -d "$HOME/Qt" ]]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [[ -n "$QT_VERSION" && -d "$HOME/Qt/$QT_VERSION/gcc_64" ]]; then
                echo "$HOME/Qt/$QT_VERSION/gcc_64"
                return
            fi
        fi
    elif [[ "$os" == "windows" ]]; then
        if [[ -d "$HOME/Qt" ]]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [[ -n "$QT_VERSION" ]]; then
                for compiler_dir in "$HOME/Qt/$QT_VERSION"/*; do
                    if [[ -d "$compiler_dir" && $(basename "$compiler_dir") =~ ^(mingw|msvc) ]]; then
                        echo "$compiler_dir"
                        return
                    fi
                done
            fi
        fi
        if [[ -d "C:/Qt" ]]; then
            QT_VERSION=$(ls -1 "C:/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [[ -n "$QT_VERSION" ]]; then
                for compiler_dir in "C:/Qt/$QT_VERSION"/*; do
                    if [[ -d "$compiler_dir" && $(basename "$compiler_dir") =~ ^(mingw|msvc) ]]; then
                        echo "$compiler_dir"
                        return
                    fi
                done
            fi
        fi
    fi
    
    echo ""
}

get_cpu_count() {
    local os=$1
    if [[ "$os" == "macos" ]]; then
        sysctl -n hw.ncpu
    elif [[ "$os" == "linux" ]]; then
        nproc
    elif [[ "$os" == "windows" ]]; then
        echo "$NUMBER_OF_PROCESSORS"
    else
        echo "4"
    fi
}

OS=$(detect_os)
echo "==> Detected OS: $OS"

if [[ "$OS" == "unknown" ]]; then
    echo "Error: Unsupported operating system"
    exit 1
fi

QT_PATH=$(find_qt_path "$OS")
if [[ -z "$QT_PATH" ]]; then
    echo "Error: Could not find Qt installation"
    echo "Please set QT_PATH environment variable or install Qt"
    exit 1
fi

echo "==> Using Qt from: $QT_PATH"

echo "==> Cleaning build directory..."
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

echo "==> Configuring with CMake..."
cd "$BUILD_DIR"
cmake -DCMAKE_PREFIX_PATH="$QT_PATH" \
      -DCMAKE_BUILD_TYPE=Release \
      ..

echo "==> Building application..."
CPU_COUNT=$(get_cpu_count "$OS")
cmake --build . --config Release -j"$CPU_COUNT"

if [[ "$OS" == "macos" ]]; then
    echo "==> Deploying Qt dependencies (macOS)..."
    "$QT_PATH/bin/macdeployqt" "$APP_NAME.app" -verbose=1
    
    echo "==> Creating DMG installer..."
    cpack -G DragNDrop
    
    echo "==> Copying installer to $INSTALLER_DIR directory..."
    cd ..
    mkdir -p "$INSTALLER_DIR"
    cp "$BUILD_DIR"/*.dmg "$INSTALLER_DIR/" 2>/dev/null || echo "Warning: No DMG file to copy"
    
    echo "==> Build and deployment complete!"
    echo ""
    echo "Application bundle: $BUILD_DIR/$APP_NAME.app"
    echo "DMG installer: $INSTALLER_DIR/"
    ls -lh "$INSTALLER_DIR"/*.dmg 2>/dev/null || echo "No DMG found"
    
elif [[ "$OS" == "linux" ]]; then
    echo "==> Deploying Qt dependencies (Linux)..."
    if command -v linuxdeployqt &> /dev/null; then
        linuxdeployqt "$APP_NAME" -verbose=1
    else
        echo "Note: linuxdeployqt not found. Skipping deployment."
        echo "Install from: https://github.com/probonopd/linuxdeployqt"
    fi
    
    echo "==> Creating DEB package..."
    cpack -G DEB
    
    echo "==> Copying installer to $INSTALLER_DIR directory..."
    cd ..
    mkdir -p "$INSTALLER_DIR"
    cp "$BUILD_DIR"/*.deb "$INSTALLER_DIR/" 2>/dev/null || echo "Warning: No DEB file to copy"
    
    echo "==> Build and deployment complete!"
    echo ""
    echo "Executable: $BUILD_DIR/$APP_NAME"
    echo "DEB package: $INSTALLER_DIR/"
    ls -lh "$INSTALLER_DIR"/*.deb 2>/dev/null || echo "No DEB found"
    
elif [[ "$OS" == "windows" ]]; then
    echo "==> Deploying Qt dependencies (Windows)..."
    "$QT_PATH/bin/windeployqt.exe" --release "$APP_NAME.exe"
    
    echo "==> Creating NSIS installer..."
    if command -v makensis &> /dev/null; then
        cpack -G NSIS
    else
        echo "Note: NSIS not found. Creating ZIP package instead."
        cpack -G ZIP
    fi
    
    echo "==> Copying installer to $INSTALLER_DIR directory..."
    cd ..
    mkdir -p "$INSTALLER_DIR"
    cp "$BUILD_DIR"/*.exe "$INSTALLER_DIR/" 2>/dev/null
    cp "$BUILD_DIR"/*.zip "$INSTALLER_DIR/" 2>/dev/null
    
    echo "==> Build and deployment complete!"
    echo ""
    echo "Executable: $BUILD_DIR/$APP_NAME.exe"
    echo "Installer: $INSTALLER_DIR/"
    ls -lh "$INSTALLER_DIR"/*.exe "$INSTALLER_DIR"/*.zip 2>/dev/null || echo "No installer found"
fi
