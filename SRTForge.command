#!/bin/zsh
set -e

cd "$(dirname "$0")"

export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"

echo "SRTForge를 빌드합니다..."
SWIFT_BUILD_DIR="${TMPDIR:-/private/tmp}/SRTForge-SwiftPM"
swift build -c release --scratch-path "$SWIFT_BUILD_DIR"
BIN_DIR="$(swift build -c release --scratch-path "$SWIFT_BUILD_DIR" --show-bin-path)"

APP="build/SRTForge.app"
MACOS="$APP/Contents/MacOS"
RESOURCES="$APP/Contents/Resources"
mkdir -p "$MACOS" "$RESOURCES"
chmod -R u+w "$APP" 2>/dev/null || true
cp "$BIN_DIR/SRTForge" "$MACOS/SRTForge"
cp "App-Info.plist" "$APP/Contents/Info.plist"
codesign --force --deep --sign - "$APP" >/dev/null

echo "SRTForge를 실행합니다..."
open "$APP"
