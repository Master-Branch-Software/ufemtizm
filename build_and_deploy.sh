#!/bin/bash

set -e

APP_NAME="UnfuckMyTimeZoneMath"
BUNDLE_NAME="Unfuck My Time Zone Math"
BUILD_DIR="build"
INSTALLER_DIR="installers"

# Extract project version from CMakeLists.txt so artifacts can be versioned
PROJECT_VERSION=$(grep -E '^project\(' CMakeLists.txt 2>/dev/null | \
    sed -E 's/.*VERSION[ ]+([^ ]+).*/\1/' )

# Default DMG filename (used on macOS)
if [[ -n "$PROJECT_VERSION" ]]; then
    DMG_FILENAME="${BUNDLE_NAME}-${PROJECT_VERSION}.dmg"
else
    DMG_FILENAME="${BUNDLE_NAME}.dmg"
fi

detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "macos"
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
        # Prefer system Qt6 for better compatibility with installed packages
        if command -v qmake6 &> /dev/null; then
            local qt_path=$(qmake6 -query QT_INSTALL_PREFIX)
            if [[ -n "$qt_path" && -d "$qt_path" ]]; then
                echo "$qt_path"
                return
            fi
        fi
        if [[ -d "$HOME/Qt" ]]; then
            QT_VERSION=$(ls -1 "$HOME/Qt" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
            if [[ -n "$QT_VERSION" && -d "$HOME/Qt/$QT_VERSION/gcc_64" ]]; then
                echo "$HOME/Qt/$QT_VERSION/gcc_64"
                return
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

# Determine CMake generator based on OS and Qt installation
cmake -DCMAKE_PREFIX_PATH="$QT_PATH" \
      -DCMAKE_BUILD_TYPE=Release \
      ..

echo "==> Building application..."
CPU_COUNT=$(get_cpu_count "$OS")
cmake --build . --config Release -j"$CPU_COUNT"

if [[ "$OS" == "macos" ]]; then
    echo "==> Ensuring app bundle has icon..."
    if [[ -f "../icon.icns" ]]; then
        mkdir -p "$BUNDLE_NAME.app/Contents/Resources"
        cp "../icon.icns" "$BUNDLE_NAME.app/Contents/Resources/icon.icns"
    fi

    echo "==> Deploying Qt dependencies (macOS via macdeployqt)..."
    # Deploy Qt frameworks and plugins into the .app bundle
    "$QT_PATH/bin/macdeployqt" "$BUNDLE_NAME.app" -verbose=1

    echo "==> Creating DMG with Applications link..."
    DMG_ROOT_DIR="dmg_root"
    rm -rf "$DMG_ROOT_DIR"
    mkdir -p "$DMG_ROOT_DIR"

    # Copy app bundle into DMG root
    cp -R "$BUNDLE_NAME.app" "$DMG_ROOT_DIR/"

    # Add /Applications symlink for drag-and-drop install UX
    ln -s /Applications "$DMG_ROOT_DIR/Applications" 2>/dev/null || true

    # Create the compressed DMG (contents: app bundle + Applications link)
    hdiutil create -volname "Unfuck My TimeZone Math" -srcfolder "$DMG_ROOT_DIR" -ov -format UDZO "$DMG_FILENAME"
    
    echo "==> Copying installer to $INSTALLER_DIR directory..."
    cd ..
    mkdir -p "$INSTALLER_DIR"
    cp "$BUILD_DIR"/*.dmg "$INSTALLER_DIR/" 2>/dev/null || echo "Warning: No DMG file to copy"
    
    echo "==> Build and deployment complete!"
    echo ""
    echo "Application bundle: $BUILD_DIR/$BUNDLE_NAME.app"
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
    echo ""
    echo "To install the DEB package:"
    echo "  sudo dpkg -i $INSTALLER_DIR/*.deb"
    echo "To update icon cache manually (if needed):"
    echo "  sudo gtk-update-icon-cache -f -t /usr/share/icons/hicolor"
    echo "  sudo update-desktop-database /usr/share/applications"
    
fi
