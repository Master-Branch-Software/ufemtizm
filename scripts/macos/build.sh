#!/bin/bash
# build.sh
#
# Configures and builds a macOS .app bundle (and optional .dmg or .pkg) for local
# testing of Qt projects. Modeled on android/build.sh.
#
# Quick start:
#   ./scripts/macos/build.sh                     # Debug .app, unsigned
#   ./scripts/macos/build.sh --release --sign    # Release .app, Developer ID signed
#   ./scripts/macos/build.sh --release --dmg --sign --notarize
#   ./scripts/macos/build.sh --release --mas --sign-mas --pkg
#
# Usage:
#   ./scripts/macos/build.sh [--clean] [--debug|--release]
#                            [--unsigned|--sign|--sign-mas]
#                            [--direct|--mas] [--dmg] [--pkg]
#                            [--notarize] [--open]
#
# Options:
#   --clean           Remove the macOS build directory before configuring.
#   --debug           Build a Debug configuration (default).
#   --release         Build a Release configuration.
#   --unsigned        Disable code signing.
#   --sign            Sign with Developer ID (direct distribution). Implies --direct.
#   --sign-mas        Sign with Mac App Distribution (Mac App Store). Implies --mas.
#   --direct          Target direct (Developer ID / notarized) distribution. Default.
#   --mas             Target Mac App Store distribution.
#   --dmg             Produce a .dmg via CPack (DragNDrop generator). Direct only.
#   --pkg             Produce a .pkg via productbuild. Required for --mas.
#   --notarize        Submit the built artifact to Apple notarization (direct only).
#   --open            Open the built .app after the build succeeds.
#
# Optional local configuration file:
#   .env.macos.local in repo root (or ENV_FILE=<path>)
#
# Environment variables (auto-discovered if possible):
#   CMAKE_GENERATOR              (optional; auto-selects Ninja, then Unix Makefiles)
#   QT_CMAKE                     (auto-discovered if unset)
#   QT_MACOS                     (derived from QT_CMAKE if unset)
#   QT_ROOT                      (derived from QT_MACOS if unset)
#   QT_VERSION                   (derived from QT_ROOT directory name if unset)
#   MACOS_BUILD_DIR              (default: build/macos-local)
#   MACOS_BUNDLE_IDENTIFIER      (optional override; default comes from CMakeLists.txt)
#   APP_BUILD_NUMBER             (optional override; default comes from CMakeLists.txt)
#   APP_VERSION_STRING           (optional override; default comes from CMakeLists.txt)
#   MACOS_SCHEME                 (optional; app target name for locating the .app)
#
# Signing environment variables for --sign (Developer ID, direct distribution):
#   APPLE_TEAM_ID                      (10-character Apple Developer Team ID)
#   MACOS_SIGN_IDENTITY                (default: "Developer ID Application")
#   MACOS_INSTALLER_SIGN_IDENTITY      (default: "Developer ID Installer")
#   MACOS_ENTITLEMENTS                 (optional path to entitlements plist)
#
# Signing environment variables for --sign-mas (Mac App Store distribution):
#   APPLE_TEAM_ID                      (10-character Apple Developer Team ID)
#   MAS_SIGN_IDENTITY                  (default: "Apple Distribution")
#   MAS_INSTALLER_SIGN_IDENTITY        (default: "3rd Party Mac Developer Installer")
#   MAS_PROVISIONING_PROFILE           (path to Mac App Distribution .provisionprofile)
#   MAS_ENTITLEMENTS                   (path to Mac App Store entitlements plist; required)
#
# Notarization variables for --notarize:
#   NOTARY_APPLE_ID            (Apple ID used for notarytool)
#   NOTARY_TEAM_ID             (defaults to APPLE_TEAM_ID)
#   NOTARY_PASSWORD            (app-specific password; or use NOTARY_KEYCHAIN_PROFILE)
#   NOTARY_KEYCHAIN_PROFILE    (notarytool keychain profile name; preferred)

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

CONFIG=""
CLEAN=false
OPEN_APP=false
SIGN_MODE="${SIGN_MODE:-}"
DIST_MODE=""
BUILD_DMG=false
BUILD_PKG=false
NOTARIZE=false

ENV_FILE="${ENV_FILE:-$PROJECT_DIR/.env.macos.local}"
if [ -f "$ENV_FILE" ]; then
  set -a
  # shellcheck disable=SC1090
  . "$ENV_FILE"
  set +a
fi

QT_CMAKE="${QT_CMAKE:-}"
QT_MACOS="${QT_MACOS:-}"
QT_ROOT="${QT_ROOT:-}"
QT_VERSION="${QT_VERSION:-}"
CMAKE_GENERATOR="${CMAKE_GENERATOR:-}"

MACOS_BUILD_DIR="${MACOS_BUILD_DIR:-}"
MACOS_BUNDLE_IDENTIFIER="${MACOS_BUNDLE_IDENTIFIER:-}"
APP_BUILD_NUMBER="${APP_BUILD_NUMBER:-}"
APP_VERSION_STRING="${APP_VERSION_STRING:-}"
MACOS_SCHEME="${MACOS_SCHEME:-}"

APPLE_TEAM_ID="${APPLE_TEAM_ID:-}"
MACOS_SIGN_IDENTITY="${MACOS_SIGN_IDENTITY:-Developer ID Application}"
MACOS_INSTALLER_SIGN_IDENTITY="${MACOS_INSTALLER_SIGN_IDENTITY:-Developer ID Installer}"
MACOS_ENTITLEMENTS="${MACOS_ENTITLEMENTS:-}"

MAS_SIGN_IDENTITY="${MAS_SIGN_IDENTITY:-Apple Distribution}"
MAS_INSTALLER_SIGN_IDENTITY="${MAS_INSTALLER_SIGN_IDENTITY:-3rd Party Mac Developer Installer}"
MAS_PROVISIONING_PROFILE="${MAS_PROVISIONING_PROFILE:-}"
MAS_ENTITLEMENTS="${MAS_ENTITLEMENTS:-}"

NOTARY_APPLE_ID="${NOTARY_APPLE_ID:-}"
NOTARY_TEAM_ID="${NOTARY_TEAM_ID:-${APPLE_TEAM_ID:-}}"
NOTARY_PASSWORD="${NOTARY_PASSWORD:-}"
NOTARY_KEYCHAIN_PROFILE="${NOTARY_KEYCHAIN_PROFILE:-}"

usage() {
  cat <<'EOF'
Usage:
  ./scripts/macos/build.sh [--clean] [--debug|--release]
                           [--unsigned|--sign|--sign-mas]
                           [--direct|--mas] [--dmg] [--pkg]
                           [--notarize] [--open]

Examples:
  ./scripts/macos/build.sh                                        # Debug .app, unsigned
  ./scripts/macos/build.sh --release --sign --dmg --notarize      # Direct, notarized .dmg
  APPLE_TEAM_ID=ABCDE12345 MAS_PROVISIONING_PROFILE=./mas.provisionprofile \
    MAS_ENTITLEMENTS=./mas.entitlements.plist \
    ./scripts/macos/build.sh --release --sign-mas --pkg           # Mac App Store .pkg
EOF
}

find_latest_qt_cmake() {
  local candidate
  candidate="$(find "$HOME/Qt" -maxdepth 4 -type f \
    \( -path "*/macos/bin/qt-cmake" \
    -o -path "*/clang_64/bin/qt-cmake" \) \
    2>/dev/null | sort -V | tail -n 1 || true)"
  if [ -n "${candidate:-}" ]; then
    printf '%s' "$candidate"
    return 0
  fi

  return 1
}

discover_qt_paths() {
  if [ -z "${QT_CMAKE:-}" ]; then
    if [ -n "${QT_MACOS:-}" ] && [ -x "${QT_MACOS}/bin/qt-cmake" ]; then
      QT_CMAKE="${QT_MACOS}/bin/qt-cmake"
    elif command -v qt-cmake >/dev/null 2>&1; then
      QT_CMAKE="$(command -v qt-cmake)"
    else
      QT_CMAKE="$(find_latest_qt_cmake || true)"
    fi
  fi

  if [ -z "${QT_CMAKE:-}" ] || [ ! -x "$QT_CMAKE" ]; then
    echo "qt-cmake not found."
    echo "Install a Qt macOS kit or set QT_CMAKE (or QT_MACOS)."
    exit 1
  fi

  if [ -z "${QT_MACOS:-}" ]; then
    QT_MACOS="$(cd "$(dirname "$QT_CMAKE")/.." && pwd)"
  fi

  if [ -z "${QT_ROOT:-}" ]; then
    QT_ROOT="$(dirname "$QT_MACOS")"
  fi

  if [ -z "${QT_VERSION:-}" ]; then
    QT_VERSION="$(basename "$QT_ROOT")"
  fi
}

select_cmake_generator() {
  if [ -n "${CMAKE_GENERATOR:-}" ]; then
    return 0
  fi

  if command -v ninja >/dev/null 2>&1; then
    CMAKE_GENERATOR="Ninja"
    return 0
  fi

  if command -v make >/dev/null 2>&1; then
    CMAKE_GENERATOR="Unix Makefiles"
    return 0
  fi

  echo "No supported CMake generator tool found."
  echo "Install ninja or make, or set CMAKE_GENERATOR explicitly."
  exit 1
}

ensure_generator_compatible_build_dir() {
  local cache_file="$MACOS_BUILD_DIR/CMakeCache.txt"

  if [ ! -f "$cache_file" ]; then
    return 0
  fi

  local cached_generator
  cached_generator="$(awk -F= '/^CMAKE_GENERATOR:INTERNAL=/{print $2; exit}' "$cache_file")"

  if [ -z "${cached_generator:-}" ]; then
    return 0
  fi

  if [ "$cached_generator" != "$CMAKE_GENERATOR" ]; then
    echo "Build directory generator mismatch detected: $cached_generator -> $CMAKE_GENERATOR"
    echo "Cleaning build directory: $MACOS_BUILD_DIR"
    rm -rf "$MACOS_BUILD_DIR"
  fi
}

locate_app_bundle() {
  local app_path

  if [ -n "${MACOS_SCHEME:-}" ]; then
    app_path="$(find "$MACOS_BUILD_DIR" -type d -name "${MACOS_SCHEME}.app" ! -path "*/_CPack_Packages/*" 2>/dev/null | sort | head -n 1 || true)"
    if [ -n "${app_path:-}" ]; then
      printf '%s' "$app_path"
      return 0
    fi
  fi

  app_path="$(find "$MACOS_BUILD_DIR" -maxdepth 4 -type d -name "*.app" ! -path "*/_CPack_Packages/*" 2>/dev/null | sort | head -n 1 || true)"
  if [ -n "${app_path:-}" ]; then
    printf '%s' "$app_path"
    return 0
  fi

  return 1
}

sign_app_bundle() {
  local app_path="$1"
  local identity="$2"
  local entitlements="$3"
  local provisioning_profile="$4"

  if [ -n "${provisioning_profile:-}" ]; then
    if [ ! -f "$provisioning_profile" ]; then
      echo "Provisioning profile not found: $provisioning_profile"
      exit 1
    fi
    cp "$provisioning_profile" "$app_path/Contents/embedded.provisionprofile"
  fi

  local codesign_args=(--force --options runtime --timestamp --deep --sign "$identity")
  if [ -n "${entitlements:-}" ]; then
    if [ ! -f "$entitlements" ]; then
      echo "Entitlements file not found: $entitlements"
      exit 1
    fi
    codesign_args+=(--entitlements "$entitlements")
  fi

  codesign "${codesign_args[@]}" "$app_path"
  codesign --verify --deep --strict --verbose=2 "$app_path"
}

build_dmg() {
  local app_path="$1"
  local dmg_dir="$MACOS_BUILD_DIR/dmg-staging"
  local app_name
  app_name="$(basename "$app_path")"

  rm -rf "$dmg_dir"
  mkdir -p "$dmg_dir"
  cp -R "$app_path" "$dmg_dir/"
  ln -s /Applications "$dmg_dir/Applications"

  local dmg_base="${app_name%.app}"
  local dmg_path="$MACOS_BUILD_DIR/${dmg_base}.dmg"
  rm -f "$dmg_path"

  hdiutil create -volname "$dmg_base" -srcfolder "$dmg_dir" -ov -format UDZO "$dmg_path"

  if [ "$SIGN_MODE" = "direct" ]; then
    codesign --force --timestamp --sign "$MACOS_SIGN_IDENTITY" "$dmg_path"
  fi

  printf '%s' "$dmg_path"
}

build_pkg() {
  local app_path="$1"
  local installer_identity="$2"
  local app_name
  app_name="$(basename "$app_path")"

  local pkg_base="${app_name%.app}"
  local pkg_path="$MACOS_BUILD_DIR/${pkg_base}.pkg"
  rm -f "$pkg_path"

  productbuild \
    --component "$app_path" /Applications \
    --sign "$installer_identity" \
    "$pkg_path"

  printf '%s' "$pkg_path"
}

notarize_artifact() {
  local artifact_path="$1"

  if ! command -v xcrun >/dev/null 2>&1; then
    echo "xcrun not found. Install Xcode command-line tools."
    exit 1
  fi

  local notary_args=(notarytool submit "$artifact_path" --wait)
  if [ -n "${NOTARY_KEYCHAIN_PROFILE:-}" ]; then
    notary_args+=(--keychain-profile "$NOTARY_KEYCHAIN_PROFILE")
  else
    if [ -z "${NOTARY_APPLE_ID:-}" ] || [ -z "${NOTARY_TEAM_ID:-}" ] || [ -z "${NOTARY_PASSWORD:-}" ]; then
      echo "Missing notarization credentials."
      echo "Set NOTARY_KEYCHAIN_PROFILE, or NOTARY_APPLE_ID + NOTARY_TEAM_ID + NOTARY_PASSWORD."
      exit 1
    fi
    notary_args+=(--apple-id "$NOTARY_APPLE_ID" --team-id "$NOTARY_TEAM_ID" --password "$NOTARY_PASSWORD")
  fi

  xcrun "${notary_args[@]}"
  xcrun stapler staple "$artifact_path"
}

for arg in "$@"; do
  case "$arg" in
    --clean)
      CLEAN=true
      ;;
    --debug)
      CONFIG="Debug"
      ;;
    --release)
      CONFIG="Release"
      ;;
    --unsigned)
      SIGN_MODE="none"
      ;;
    --sign)
      SIGN_MODE="direct"
      DIST_MODE="direct"
      ;;
    --sign-mas)
      SIGN_MODE="mas"
      DIST_MODE="mas"
      ;;
    --direct)
      DIST_MODE="direct"
      ;;
    --mas)
      DIST_MODE="mas"
      ;;
    --dmg)
      BUILD_DMG=true
      ;;
    --pkg)
      BUILD_PKG=true
      ;;
    --notarize)
      NOTARIZE=true
      ;;
    --open)
      OPEN_APP=true
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $arg"
      usage
      exit 1
      ;;
  esac
done

CONFIG="${CONFIG:-Debug}"
DIST_MODE="${DIST_MODE:-direct}"
SIGN_MODE="${SIGN_MODE:-none}"
MACOS_BUILD_DIR="${MACOS_BUILD_DIR:-$PROJECT_DIR/build/macos-local}"

if [[ "$SIGN_MODE" != "none" && "$SIGN_MODE" != "direct" && "$SIGN_MODE" != "mas" ]]; then
  echo "Invalid SIGN_MODE value: $SIGN_MODE"
  echo "Supported values: none, direct, mas"
  exit 1
fi

if [[ "$DIST_MODE" != "direct" && "$DIST_MODE" != "mas" ]]; then
  echo "Invalid DIST_MODE value: $DIST_MODE"
  echo "Supported values: direct, mas"
  exit 1
fi

if [ "$DIST_MODE" = "mas" ] && [ "$BUILD_DMG" = true ]; then
  echo "--dmg is not supported for Mac App Store distribution. Use --pkg."
  exit 1
fi

if [ "$DIST_MODE" = "mas" ] && [ "$NOTARIZE" = true ]; then
  echo "--notarize is not supported for Mac App Store distribution."
  exit 1
fi

if [ "$SIGN_MODE" = "mas" ] && [ -z "${MAS_ENTITLEMENTS:-}" ]; then
  echo "Mac App Store signing requires MAS_ENTITLEMENTS."
  exit 1
fi

if [ "$NOTARIZE" = true ] && [ "$SIGN_MODE" = "none" ]; then
  echo "Notarization requires a signed artifact. Use --sign."
  exit 1
fi

discover_qt_paths
select_cmake_generator
ensure_generator_compatible_build_dir

if [ "$CLEAN" = true ] && [ -d "$MACOS_BUILD_DIR" ]; then
  echo "Cleaning build directory: $MACOS_BUILD_DIR"
  rm -rf "$MACOS_BUILD_DIR"
fi

echo "Mode: macOS app ($DIST_MODE)"
echo "Config: $CONFIG"
echo "Build dir: $MACOS_BUILD_DIR"
if [ -n "${MACOS_BUNDLE_IDENTIFIER:-}" ]; then
  echo "Bundle identifier override: $MACOS_BUNDLE_IDENTIFIER"
else
  echo "Bundle identifier: from CMake default"
fi
if [ -n "${APP_VERSION_STRING:-}" ]; then
  echo "Version override: $APP_VERSION_STRING"
else
  echo "Version: from CMake default"
fi
if [ -n "${APP_BUILD_NUMBER:-}" ]; then
  echo "Build number override: $APP_BUILD_NUMBER"
else
  echo "Build number: from CMake default"
fi
echo "Qt: $QT_VERSION"
echo "Qt cmake: $QT_CMAKE"
echo "CMake generator: $CMAKE_GENERATOR"
echo "Signing mode: $SIGN_MODE"
echo "Env file: $ENV_FILE"

configure_args=(
  -S "$PROJECT_DIR"
  -B "$MACOS_BUILD_DIR"
  -G "$CMAKE_GENERATOR"
  -DCMAKE_BUILD_TYPE="$CONFIG"
)

if [ -n "${MACOS_BUNDLE_IDENTIFIER:-}" ]; then
  configure_args+=(-DMACOSX_BUNDLE_GUI_IDENTIFIER="$MACOS_BUNDLE_IDENTIFIER")
fi

if [ -n "${APP_BUILD_NUMBER:-}" ]; then
  configure_args+=(-DAPP_BUILD_NUMBER="$APP_BUILD_NUMBER")
fi

if [ -n "${APP_VERSION_STRING:-}" ]; then
  configure_args+=(-DAPP_VERSION_STRING="$APP_VERSION_STRING")
fi

if [ -n "${APPLE_TEAM_ID:-}" ]; then
  configure_args+=(-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="$APPLE_TEAM_ID")
fi

"$QT_CMAKE" "${configure_args[@]}"
cmake --build "$MACOS_BUILD_DIR" --config "$CONFIG" --parallel

APP_PATH="$(locate_app_bundle || true)"
if [ -z "${APP_PATH:-}" ]; then
  echo "No .app bundle found under: $MACOS_BUILD_DIR"
  exit 1
fi

echo ""
echo ".app built at:"
echo "  $APP_PATH"

if [ "$SIGN_MODE" = "direct" ]; then
  if [ -z "${APPLE_TEAM_ID:-}" ]; then
    echo "Missing APPLE_TEAM_ID for --sign."
    exit 1
  fi
  echo "Signing with: $MACOS_SIGN_IDENTITY"
  sign_app_bundle "$APP_PATH" "$MACOS_SIGN_IDENTITY" "${MACOS_ENTITLEMENTS:-}" ""
elif [ "$SIGN_MODE" = "mas" ]; then
  if [ -z "${APPLE_TEAM_ID:-}" ]; then
    echo "Missing APPLE_TEAM_ID for --sign-mas."
    exit 1
  fi
  if [ -z "${MAS_PROVISIONING_PROFILE:-}" ]; then
    echo "Missing MAS_PROVISIONING_PROFILE for --sign-mas."
    exit 1
  fi
  echo "Signing with: $MAS_SIGN_IDENTITY"
  sign_app_bundle "$APP_PATH" "$MAS_SIGN_IDENTITY" "$MAS_ENTITLEMENTS" "$MAS_PROVISIONING_PROFILE"
fi

DMG_PATH=""
if [ "$BUILD_DMG" = true ]; then
  echo "Building .dmg..."
  DMG_PATH="$(build_dmg "$APP_PATH")"
  echo ".dmg built at:"
  echo "  $DMG_PATH"
fi

PKG_PATH=""
if [ "$BUILD_PKG" = true ]; then
  if [ "$DIST_MODE" = "mas" ]; then
    echo "Building .pkg with: $MAS_INSTALLER_SIGN_IDENTITY"
    PKG_PATH="$(build_pkg "$APP_PATH" "$MAS_INSTALLER_SIGN_IDENTITY")"
  else
    if [ "$SIGN_MODE" = "none" ]; then
      echo "--pkg for direct distribution requires --sign."
      exit 1
    fi
    echo "Building .pkg with: $MACOS_INSTALLER_SIGN_IDENTITY"
    PKG_PATH="$(build_pkg "$APP_PATH" "$MACOS_INSTALLER_SIGN_IDENTITY")"
  fi
  echo ".pkg built at:"
  echo "  $PKG_PATH"
fi

if [ "$NOTARIZE" = true ]; then
  NOTARIZE_ARTIFACT=""
  if [ -n "${DMG_PATH:-}" ]; then
    NOTARIZE_ARTIFACT="$DMG_PATH"
  elif [ -n "${PKG_PATH:-}" ]; then
    NOTARIZE_ARTIFACT="$PKG_PATH"
  else
    NOTARIZE_ARTIFACT="$APP_PATH"
  fi
  echo "Notarizing: $NOTARIZE_ARTIFACT"
  notarize_artifact "$NOTARIZE_ARTIFACT"
fi

if [ "$OPEN_APP" = true ]; then
  open "$APP_PATH"
fi

if [ "$SIGN_MODE" = "none" ]; then
  echo ""
  echo "Note: signing is disabled."
  echo "Use --sign for Developer ID distribution or --sign-mas for Mac App Store."
fi
