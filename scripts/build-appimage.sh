#!/usr/bin/env bash
# Build xquest-x86_64.AppImage
# Run from repo root after scripts/fetch-assets.sh
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-appimage"
APPDIR="$BUILD_DIR/XQuest.AppDir"
OUT="$REPO_ROOT/xquest-x86_64.AppImage"

DATA_DEST="/usr/share/games/xquest"

# ── Fetch appimagetool if not present ────────────────────────────────────────
APPIMAGETOOL="${APPIMAGETOOL:-}"
if [[ -z "$APPIMAGETOOL" ]]; then
    TOOL="$BUILD_DIR/appimagetool-x86_64.AppImage"
    if [[ ! -x "$TOOL" ]]; then
        echo "Downloading appimagetool..."
        mkdir -p "$BUILD_DIR"
        curl -fsSL -o "$TOOL" \
            "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
        chmod +x "$TOOL"
    fi
    APPIMAGETOOL="$TOOL"
fi

# ── Check assets ─────────────────────────────────────────────────────────────
if [[ ! -f "$REPO_ROOT/assets/xquest.gfx" ]]; then
    echo "ERROR: assets/ not populated — run scripts/fetch-assets.sh first" >&2
    exit 1
fi

# ── CMake build (Release, install into AppDir) ───────────────────────────────
echo "Building..."
cmake -B "$BUILD_DIR/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DXQUEST_ASSET_DIR="$DATA_DEST" \
    -DXQUEST_DATA_SOURCE="$REPO_ROOT/assets" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    "$REPO_ROOT" -DCMAKE_INSTALL_RPATH='$ORIGIN/../lib'
cmake --build "$BUILD_DIR/cmake" --parallel

DESTDIR="$APPDIR" cmake --install "$BUILD_DIR/cmake"

# ── AppDir skeleton ──────────────────────────────────────────────────────────
# .desktop at root (required by appimagetool)
cp "$REPO_ROOT/debian/xquest.desktop" "$APPDIR/xquest.desktop"

# Icon at root (appimagetool looks for <AppName>.png or .DirIcon)
cp "$REPO_ROOT/assets/icons/xquest.png" "$APPDIR/xquest.png"

# AppRun script — sets XQUEST_DATA_DIR so the binary finds data inside the image
cat > "$APPDIR/AppRun" << 'APPRUN'
#!/bin/sh
export XQUEST_DATA_DIR="$APPDIR/usr/share/games/xquest"
exec "$APPDIR/usr/games/xquest" "$@"
APPRUN
chmod +x "$APPDIR/AppRun"

# ── Bundle SDL2 ──────────────────────────────────────────────────────────────
# Copy libSDL2 so the AppImage runs on systems without it installed
LIBDIR="$APPDIR/usr/lib"
mkdir -p "$LIBDIR"
for lib in $(ldd "$APPDIR/usr/games/xquest" | awk '/libSDL2/{print $3}'); do
    [[ -f "$lib" ]] && cp -v "$lib" "$LIBDIR/"
done

# ── Pack ──────────────────────────────────────────────────────────────────────
echo "Packing AppImage..."
ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$OUT"
echo ""
echo "Done: $OUT"
