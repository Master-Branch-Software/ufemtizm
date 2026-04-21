#!/bin/bash

set -e

APP_NAME="UnfuckMyTimeZoneMath"
BUNDLE_NAME="Unfuck My Time Zone Math"
BUILD_DIR="build"
INSTALLER_DIR="installers"

# Extract project version from CMakeLists.txt so artifacts can be versioned
# CMakeLists.txt uses a multi-line project() definition, so search for the
# VERSION line separately and grab the value after the keyword.
PROJECT_VERSION=$(grep -E '^[[:space:]]*VERSION[[:space:]]+' CMakeLists.txt 2>/dev/null | \
    sed -E 's/^[[:space:]]*VERSION[[:space:]]+([^[:space:]]+).*/\1/' )

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
        # 1. Try qtpaths6 (preferred on many distros)
        if command -v qtpaths6 &> /dev/null; then
            local qt_path=$(qtpaths6 --install-prefix 2>/dev/null || qtpaths6 --query QT_INSTALL_PREFIX 2>/dev/null)
            if [[ -n "$qt_path" && -d "$qt_path" ]]; then
                echo "$qt_path"
                return
            fi
        fi

        # 2. Fallback to qmake6 if available
        if command -v qmake6 &> /dev/null; then
            local qt_path=$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null)
            if [[ -n "$qt_path" && -d "$qt_path" ]]; then
                echo "$qt_path"
                return
            fi
        fi

        # 3. Look for Qt6 CMake packages in common system prefixes
        for prefix in /usr /usr/local; do
            if [[ -d "$prefix/lib/cmake/Qt6" || -d "$prefix/lib64/cmake/Qt6" || -d "$prefix/lib/x86_64-linux-gnu/cmake/Qt6" ]]; then
                echo "$prefix"
                return
            fi
        done

        # 4. Fallback to Qt Online Installer layout under $HOME/Qt or /opt/Qt
        for base in "$HOME/Qt" "/opt/Qt"; do
            if [[ -d "$base" ]]; then
                QT_VERSION=$(ls -1 "$base" | grep -E '^[0-9]+\.[0-9]+\.[0-9]+$' | sort -V | tail -1)
                if [[ -n "$QT_VERSION" && -d "$base/$QT_VERSION/gcc_64" ]]; then
                    echo "$base/$QT_VERSION/gcc_64"
                    return
                fi
            fi
        done
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

# Handle --snap flag: build a snap package via snapcraft, bypassing the cmake flow
for arg in "$@"; do
    if [[ "$arg" == "--snap" ]]; then
        if [[ "$OS" != "linux" ]]; then
            echo "Error: Snap builds are only supported on Linux"
            exit 1
        fi

        if ! command -v snapcraft &> /dev/null; then
            echo "Error: snapcraft not found."
            echo "Install with: sudo snap install snapcraft --classic"
            exit 1
        fi

        echo "==> Building snap package..."

        # KDE neon sets ID=neon in /etc/os-release; snapcraft requires ID=ubuntu.
        # Temporarily patch it for the duration of the build.
        OS_RELEASE="/etc/os-release"
        OS_PATCHED=false
        if grep -q '^ID=neon' "$OS_RELEASE" 2>/dev/null; then
            echo "    Patching /etc/os-release for snapcraft compatibility..."
            sudo cp "$OS_RELEASE" "${OS_RELEASE}.snap_bak"
            sudo sed -i 's/^ID=neon/ID=ubuntu/' "$OS_RELEASE"
            OS_PATCHED=true
        fi

        restore_os_release() {
            if [[ "$OS_PATCHED" == "true" ]]; then
                sudo mv "${OS_RELEASE}.snap_bak" "$OS_RELEASE" 2>/dev/null || true
            fi
        }
        trap restore_os_release EXIT INT TERM

        snapcraft --destructive-mode
        restore_os_release
        trap - EXIT INT TERM

        mkdir -p "$INSTALLER_DIR"
        SNAP_FILE=$(ls *.snap 2>/dev/null | head -1)

        if [[ -n "$SNAP_FILE" ]]; then
            cp "$SNAP_FILE" "$INSTALLER_DIR/"
            echo ""
            echo "==> Snap build complete!"
            echo "Snap package: $INSTALLER_DIR/$SNAP_FILE"
            echo ""
            echo "To install (local testing only):"
            echo "  sudo snap install $INSTALLER_DIR/$SNAP_FILE --dangerous"
        else
            echo "Warning: No .snap file found after build"
        fi
        exit 0
    fi
done

# If QT_PATH is already defined
# respect it and do not override it with auto-detection.
if [[ -n "$QT_PATH" && -d "$QT_PATH" ]]; then
    echo "==> Using Qt from QT_PATH environment variable: $QT_PATH"
else
    QT_PATH=$(find_qt_path "$OS")
fi

if [[ -z "$QT_PATH" ]]; then
    echo "Error: Could not find Qt installation"
    echo "Please set QT_PATH environment variable or install Qt6 development packages"
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
    echo "==> Bundling Qt dependencies..."
    
    # Prepare Qt libraries directory
    QT_LIBS_DIR="$PWD/qt_deploy"
    rm -rf "$QT_LIBS_DIR"
    mkdir -p "$QT_LIBS_DIR/lib"
    mkdir -p "$QT_LIBS_DIR/plugins/platforms"
    
    # Copy Qt libraries that the app depends on
    QT_LIBS=$(ldd "$APP_NAME" | grep -E 'libQt6|libicu' | awk '{print $3}' | grep -v '^$')
    for lib in $QT_LIBS; do
        if [[ -f "$lib" ]]; then
            cp -L "$lib" "$QT_LIBS_DIR/lib/"
        fi
    done
    
    # Copy Qt plugins
    if [[ -f "$QT_PATH/plugins/platforms/libqxcb.so" ]]; then
        cp "$QT_PATH/plugins/platforms/libqxcb.so" "$QT_LIBS_DIR/plugins/platforms/"
        # Copy xcb plugin dependencies
        XCB_LIBS=$(ldd "$QT_PATH/plugins/platforms/libqxcb.so" | grep -E 'libQt6' | awk '{print $3}' | grep -v '^$')
        for lib in $XCB_LIBS; do
            if [[ -f "$lib" ]]; then
                cp -L "$lib" "$QT_LIBS_DIR/lib/" 2>/dev/null || true
            fi
        done
    fi
    
    # Set RPATH on all Qt libraries
    for lib in "$QT_LIBS_DIR/lib/"*.so*; do
        if [[ -f "$lib" ]]; then
            patchelf --set-rpath '$ORIGIN' "$lib" 2>/dev/null || true
        fi
    done
    
    # Set RPATH on Qt plugins
    for plugin in "$QT_LIBS_DIR/plugins/platforms/"*.so; do
        if [[ -f "$plugin" ]]; then
            patchelf --set-rpath '$ORIGIN/../../lib' "$plugin" 2>/dev/null || true
        fi
    done
    
    # Create wrapper script
    mkdir -p "$QT_LIBS_DIR/wrapper"
    cat > "$QT_LIBS_DIR/wrapper/$APP_NAME" << 'WRAPPER_EOF'
#!/bin/bash
export QT_PLUGIN_PATH="/usr/lib/UnfuckMyTimeZoneMath/plugins"
exec "/usr/lib/UnfuckMyTimeZoneMath/UnfuckMyTimeZoneMath.bin" "$@"
WRAPPER_EOF
    chmod +x "$QT_LIBS_DIR/wrapper/$APP_NAME"
    
    echo "==> Creating DEB package with CPack..."
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
