#!/usr/bin/env bash
# Build XQuest.app (with bundled game data - see assets/README) and package
# it as a .dmg on macOS.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-macos"
APP="$BUILD_DIR/XQuest.app"
VERSION="${XQUEST_VERSION:-0.0.0}"
DMG="$REPO_ROOT/XQuest-${VERSION}-macos.dmg"

rm -rf "$BUILD_DIR" "$DMG"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources/data"

cmake -B "$BUILD_DIR/cmake" -DCMAKE_BUILD_TYPE=Release "$REPO_ROOT"
cmake --build "$BUILD_DIR/cmake" --parallel

# Real binary goes in as xquest-bin; CFBundleExecutable is a tiny launcher
# script (below) that points XQUEST_DATA_DIR at the bundle's own Resources,
# so the .app plays correctly no matter where the user drags it.
cp "$BUILD_DIR/cmake/xquest" "$APP/Contents/MacOS/xquest-bin"
for f in xquest.gfx xquest.enm xquest.fnt xquest2.fnt xquest.snd startpic.raw; do
    cp "$REPO_ROOT/assets/$f" "$APP/Contents/Resources/data/$f"
done
if [[ -f "$REPO_ROOT/assets/icons/xquest_256.png" ]]; then
    cp "$REPO_ROOT/assets/icons/xquest_256.png" "$APP/Contents/Resources/xquest.png"
fi

cat > "$APP/Contents/MacOS/xquest" << 'LAUNCHER'
#!/bin/sh
DIR="$(cd "$(dirname "$0")" && pwd)"
export XQUEST_DATA_DIR="${XQUEST_DATA_DIR:-$DIR/../Resources/data}"
exec "$DIR/xquest-bin" "$@"
LAUNCHER
chmod +x "$APP/Contents/MacOS/xquest"

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
SDL2_LIB="$(otool -L "$APP/Contents/MacOS/xquest-bin" | awk '/libSDL2.*dylib/{print $1; exit}')"
if [[ -n "${SDL2_LIB:-}" && -f "$SDL2_LIB" ]]; then
    mkdir -p "$APP/Contents/Frameworks"
    cp "$SDL2_LIB" "$APP/Contents/Frameworks/"
    install_name_tool -change "$SDL2_LIB" "@executable_path/../Frameworks/$(basename "$SDL2_LIB")" \
        "$APP/Contents/MacOS/xquest-bin"
fi

hdiutil create -volname "XQuest" -srcfolder "$APP" -ov -format UDZO "$DMG"
echo "Done: $DMG"
