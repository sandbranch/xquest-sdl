#!/usr/bin/env python3
"""Convert xquest VGA-planar .pbm files to simple palette-indexed .raw files.

Output format (little-endian):
  [width : u16][height : u16][width * height bytes of palette indices, row-major]

Usage:
  python3 tools/convert_pbm.py ../xquest/startpic.pbm [more.pbm ...]

Each input produces a .raw file next to the original.
"""

import sys
import struct
import os


def decode_pbm(data: bytes) -> tuple[int, int, bytes]:
    """Decode a VGA-planar PBM byte string to (width, height, pixels).

    Format: one or more chunks, each [bmwidth:u8][height:u8][bmwidth*4*height bytes].
    Within each chunk, a row is stored as four consecutive runs of bmwidth bytes,
    one per VGA plane (plane 0 = pixels 0,4,8,…; plane 1 = 1,5,9,…; etc.).
    Multiple chunks with the same bmwidth are stacked vertically.
    """
    if len(data) < 2:
        raise ValueError("file too short")

    bmw = data[0]
    w   = bmw * 4

    # Walk chunks to find total height
    total_h = 0
    pos = 0
    while pos + 2 <= len(data):
        if data[pos] != bmw:
            break
        ch         = data[pos + 1]
        chunk_size = bmw * 4 * ch
        if pos + 2 + chunk_size > len(data):
            break
        total_h += ch
        pos     += 2 + chunk_size

    if total_h == 0 or w == 0:
        raise ValueError(f"no valid chunks found (bmw={bmw}, w={w})")

    pixels = bytearray(w * total_h)

    pos = 0
    dy  = 0
    while pos + 2 <= len(data) and dy < total_h:
        ch    = data[pos + 1]
        chunk = data[pos + 2 :]
        for y in range(ch):
            for x in range(w):
                plane = x % 4
                b     = x // 4
                src   = plane * bmw * ch + y * bmw + b   # plane-major order
                pixels[(dy + y) * w + x] = chunk[src]
        dy  += ch
        pos += 2 + bmw * 4 * ch

    return w, total_h, bytes(pixels)


def convert(src: str, dst: str | None = None) -> None:
    if dst is None:
        dst = os.path.splitext(src)[0] + ".raw"

    with open(src, "rb") as f:
        data = f.read()

    w, h, pixels = decode_pbm(data)

    with open(dst, "wb") as f:
        f.write(struct.pack("<HH", w, h))
        f.write(pixels)

    print(f"{src}  ->  {dst}  ({w}x{h}, {len(pixels)} px)")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    for path in sys.argv[1:]:
        convert(path)
