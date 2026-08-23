#!/usr/bin/env bash
# Build an engine-only .deb (no copyrighted game data — see assets/README).
# For a full package with bundled data, use `dpkg-buildpackage` with debian/
# after running scripts/fetch-assets.sh.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build-deb"
VERSION="${XQUEST_VERSION:-0.0.0}"
ARCH="$(dpkg --print-architecture 2>/dev/null || echo amd64)"
PKGROOT="$BUILD_DIR/xquest_${VERSION}_${ARCH}"

rm -rf "$BUILD_DIR"
mkdir -p "$PKGROOT/DEBIAN"

cmake -B "$BUILD_DIR/cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
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
Description: XQuest arcade shooter — SDL2 port for Linux (engine only)
 A faithful SDL2 port of Mark Mackey's 1994 DOS shareware classic XQuest.
 This package contains the game engine only — it does not include the
 original copyrighted game data. Point the XQUEST_DATA_DIR environment
 variable at your own legally-obtained copy of the XQuest 1.3 data files.
EOF

dpkg-deb --build --root-owner-group "$PKGROOT"
echo "Done: ${PKGROOT}.deb"
