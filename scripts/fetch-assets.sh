#!/usr/bin/env bash
# Populate assets/ from the adjacent ../xquest/ original distribution.
# Run this once before building or packaging.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
SRC="${REPO_ROOT}/../xquest"
DST="${REPO_ROOT}/assets"

if [[ ! -d "$SRC" ]]; then
    echo "ERROR: Original XQuest directory not found at $SRC" >&2
    echo "       Set XQUEST_SRC env var to point to the original distribution." >&2
    SRC="${XQUEST_SRC:-}"
    [[ -d "$SRC" ]] || exit 1
fi

mkdir -p "$DST"

for f in xquest.gfx xquest.enm xquest.fnt xquest2.fnt xquest.snd startpic.pbm; do
    if [[ -f "$SRC/$f" ]]; then
        cp -v "$SRC/$f" "$DST/$f"
    else
        echo "WARNING: $SRC/$f not found, skipping" >&2
    fi
done

# Generate startpic.raw from startpic.pbm
if [[ -f "$DST/startpic.pbm" ]]; then
    echo "Generating assets/startpic.raw ..."
    python3 "$REPO_ROOT/tools/convert_pbm.py" "$DST/startpic.pbm"
fi

echo "Assets ready in $DST"
