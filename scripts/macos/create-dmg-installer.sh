#!/bin/bash
# create-dmg-installer.sh - Create DMG installer with custom background
#
# Usage: ./scripts/package/create-dmg-installer.sh <app_path> <output_dmg_name> [--open]
#
# Arguments:
#   app_path         Path to app bundle
#   output_dmg_name  Name of output DMG file (e.g., OpenWorkspace-1.0.0.dmg)
#   --open           Optional: open the DMG after creation
#
# Example:
#   ./scripts/package/create-dmg-installer.sh /path/to/OpenWorkspace.app OpenWorkspace-1.0.0.dmg
#   ./scripts/package/create-dmg-installer.sh /path/to/OpenWorkspace.app OpenWorkspace-test.dmg --open
#
# Optional environment variables:
#   OWS_DMG_VOLUME_NAME  Volume name shown when mounting DMG (default: OpenWorkspace)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

# Parse arguments
if [ $# -lt 2 ]; then
    echo "Usage: $0 <app_path> <output_dmg_name> [--open]"
    echo ""
    echo "Arguments:"
    echo "  app_path         Path to app bundle"
    echo "  output_dmg_name  Name of output DMG file"
    echo "  --open           Optional: open the DMG after creation"
    exit 1
fi

APP_PATH="$1"
OUTPUT_DMG_NAME="$2"
OPEN_DMG=false
APP_BUNDLE_NAME="$(basename "$APP_PATH")"

if [ "${3:-}" = "--open" ]; then
    OPEN_DMG=true
elif [ -n "${3:-}" ]; then
    echo "Error: Unknown third argument '$3' (expected --open)"
    exit 1
fi

# Validate app path
if [ ! -d "$APP_PATH" ]; then
    echo "Error: App not found at $APP_PATH"
    exit 1
fi

# Check for create-dmg tool
if ! command -v create-dmg >/dev/null 2>&1; then
    echo "Error: create-dmg not found. Install: brew install create-dmg"
    exit 1
fi

# Create DMG in the same directory as the app
APP_DIR="$(dirname "$APP_PATH")"
cd "$APP_DIR"
rm -f "$OUTPUT_DMG_NAME"

echo "Building DMG..."
create-dmg \
    --volname "$OUTPUT_DMG_NAME" \
    --window-pos 200 120 \
    --window-size 675 480 \
    --icon-size 100 \
    --icon "$APP_BUNDLE_NAME" 220 290 \
    --hide-extension "$APP_BUNDLE_NAME" \
    --app-drop-link 460 290 \
    --no-internet-enable \
    "$OUTPUT_DMG_NAME" \
    "$APP_BUNDLE_NAME"

if [ ! -f "$OUTPUT_DMG_NAME" ]; then
    echo "Error: DMG creation failed"
    exit 1
fi

echo "Created: $(pwd)/$OUTPUT_DMG_NAME"

if [ "$OPEN_DMG" = true ]; then
    open "$OUTPUT_DMG_NAME"
fi
