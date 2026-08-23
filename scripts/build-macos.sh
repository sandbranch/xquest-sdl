#!/usr/bin/env bash
# Build XQuest.app and package it as a .dmg on macOS.
# Engine-only — no copyrighted game data bundled (see assets/README).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-macos"
APP="$BUILD_DIR/XQuest.app"
VERSION="${XQUEST_VERSION:-0.0.0}"
DMG="$REPO_ROOT/XQuest-${VERSION}-macos.dmg"

rm -rf "$BUILD_DIR" "$DMG"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"

cmake -B "$BUILD_DIR/cmake" -DCMAKE_BUILD_TYPE=Release "$REPO_ROOT"
cmake --build "$BUILD_DIR/cmake" --parallel

cp "$BUILD_DIR/cmake/xquest" "$APP/Contents/MacOS/xquest"
if [[ -f "$REPO_ROOT/assets/icons/xquest_256.png" ]]; then
    cp "$REPO_ROOT/assets/icons/xquest_256.png" "$APP/Contents/Resources/xquest.png"
fi

cat > "$APP/Contents/Info.plist" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key><string>XQuest</string>
    <key>CFBundleDisplayName</key><string>XQuest</string>
    <key>CFBundleIdentifier</key><string>com.sandbranch.xquest</string>
    <key>CFBundleVersion</key><string>${VERSION}</string>
    <key>CFBundleShortVersionString</key><string>${VERSION}</string>
    <key>CFBundleExecutable</key><string>xquest</string>
    <key>CFBundlePackageType</key><string>APPL</string>
    <key>LSMinimumSystemVersion</key><string>11.0</string>
    <key>NSHighResolutionCapable</key><true/>
</dict>
</plist>
EOF

# Bundle SDL2.framework/dylib so the app runs without Homebrew SDL2 installed.
SDL2_LIB="$(otool -L "$APP/Contents/MacOS/xquest" | awk '/libSDL2.*dylib/{print $1; exit}')"
if [[ -n "${SDL2_LIB:-}" && -f "$SDL2_LIB" ]]; then
    mkdir -p "$APP/Contents/Frameworks"
    cp "$SDL2_LIB" "$APP/Contents/Frameworks/"
    install_name_tool -change "$SDL2_LIB" "@executable_path/../Frameworks/$(basename "$SDL2_LIB")" \
        "$APP/Contents/MacOS/xquest"
fi

hdiutil create -volname "XQuest" -srcfolder "$APP" -ov -format UDZO "$DMG"
echo "Done: $DMG"
