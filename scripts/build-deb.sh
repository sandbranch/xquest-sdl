#!/usr/bin/env bash
# Build a complete, playable .deb — the SDL2 engine plus the bundled
# XQuest game data (see assets/README: Mark Mackey's license permits this).
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-deb"
DATA_DEST="/usr/share/games/xquest"

RAW_VERSION="${XQUEST_VERSION:-0.0.0}"
RAW_VERSION="${RAW_VERSION#v}"
# Debian policy: upstream_version must start with a digit.
if [[ "$RAW_VERSION" =~ ^[0-9] ]]; then
    VERSION="$RAW_VERSION"
else
    VERSION="0.0.0+${RAW_VERSION}"
fi

ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
PKGROOT="$BUILD_DIR/xquest_${VERSION}_${ARCH}"

rm -rf "$BUILD_DIR"
mkdir -p "$PKGROOT/DEBIAN"

cmake -B "$BUILD_DIR/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DXQUEST_ASSET_DIR="$DATA_DEST" \
    -DXQUEST_DATA_SOURCE="$REPO_ROOT/assets" \
    "$REPO_ROOT"
cmake --build "$BUILD_DIR/cmake" --parallel
DESTDIR="$PKGROOT" cmake --install "$BUILD_DIR/cmake"

cat > "$PKGROOT/DEBIAN/control" << EOF
Package: xquest
Version: ${VERSION}
Section: games
Priority: optional
Architecture: ${ARCH}
Depends: libsdl2-2.0-0
Maintainer: sandbranch <sandquist@gmail.com>
Homepage: https://github.com/sandbranch/xquest-sdl
Description: XQuest arcade shooter — SDL2 port for Linux
 A faithful SDL2 port of Mark Mackey's 1994 DOS shareware classic XQuest,
 including the original game data, redistributed under Mark Mackey's
 freeware license (see /usr/share/doc/xquest/copyright).
EOF

dpkg-deb --build --root-owner-group "$PKGROOT"
echo "Done: ${PKGROOT}.deb"
