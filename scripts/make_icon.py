#!/usr/bin/env python3
"""Generate xquest.png icons from the ship sprite in sprites.json."""

import json
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow not found - pip install Pillow")

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_DIR   = SCRIPT_DIR.parent
ASSETS_DIR = REPO_DIR / "assets"
OUT_DIR    = REPO_DIR / "assets" / "icons"

# VGA palette (6-bit, 0-63) - copied verbatim from src/assets.c
XQ_PAL_VGA = [
    (0,0,0),(2,2,2),(4,4,4),(6,6,6),(8,8,8),(10,10,10),(12,12,12),(14,14,14),
    (16,16,16),(18,18,18),(20,20,20),(22,22,22),(24,24,24),(26,26,26),(28,28,28),(30,30,30),
    (32,32,32),(34,34,34),(36,36,36),(38,38,38),(40,40,40),(42,42,42),(44,44,44),(46,46,46),
    (48,48,48),(50,50,50),(52,52,52),(54,54,54),(56,56,56),(58,58,58),(60,60,60),(63,63,63),
    (0,0,6),(0,0,14),(0,0,22),(0,0,30),(0,0,38),(0,0,46),(0,0,54),(0,0,63),
    (6,6,63),(14,14,63),(22,22,63),(30,30,63),(38,38,63),(46,46,63),(54,54,63),(6,0,0),
    (14,0,0),(22,0,0),(30,0,0),(38,0,0),(46,0,0),(54,0,0),(63,0,0),(63,6,6),
    (63,14,14),(63,22,22),(63,30,30),(63,38,38),(63,46,46),(63,54,54),(0,6,0),(0,14,0),
    (0,22,0),(0,30,0),(0,38,0),(0,46,0),(0,54,0),(0,63,0),(6,63,6),(14,63,14),
    (22,63,22),(30,63,30),(38,63,38),(46,63,46),(54,63,54),(6,0,6),(14,0,14),(22,0,22),
    (30,0,30),(38,0,38),(46,0,46),(54,0,54),(63,0,63),(63,6,63),(63,14,63),(63,22,63),
    (63,30,63),(63,38,63),(63,46,63),(63,54,63),(6,6,0),(14,14,0),(22,22,0),(30,30,0),
    (38,38,0),(46,46,0),(54,54,0),(63,63,0),(63,63,6),(63,63,14),(63,63,22),(63,63,30),
    (63,63,38),(63,63,46),(63,63,54),(0,6,6),(0,14,14),(0,22,22),(0,30,30),(0,38,38),
    (0,46,46),(0,54,54),(0,63,63),(6,63,63),(14,63,63),(22,63,63),(30,63,63),(38,63,63),
    (46,63,63),(54,63,63),(6,6,2),(14,14,6),(22,22,10),(30,30,14),(38,38,18),(46,46,22),
    (54,54,26),(63,63,30),(63,63,34),(63,63,38),(63,63,42),(63,63,46),(63,63,50),(63,63,54),
    (63,63,58),(2,6,6),(6,14,14),(10,22,22),(14,30,30),(18,38,38),(22,46,46),(26,54,54),
    (30,63,63),(34,63,63),(38,63,63),(42,63,63),(46,63,63),(50,63,63),(54,63,63),(58,63,63),
    (6,2,6),(14,6,14),(22,10,22),(30,14,30),(38,18,38),(46,22,46),(54,26,54),(63,30,63),
    (63,34,63),(63,38,63),(63,42,63),(63,46,63),(63,50,63),(63,54,63),(63,58,63),(57,6,0),
    (51,12,0),(45,18,0),(38,25,0),(32,31,0),(26,37,0),(19,44,0),(13,50,0),(7,56,0),
    (0,63,0),(0,57,6),(0,51,12),(0,45,18),(0,39,24),(0,32,31),(0,26,37),(0,20,43),
    (0,14,49),(0,7,56),(0,0,63),(45,45,45),(23,23,23),(45,0,0),(23,0,0),(0,0,0),
    (0,0,0),(35,1,1),(46,22,22),(10,0,0),(34,15,15),(63,2,2),(21,0,0),(63,14,14),
    (63,30,30),(63,10,10),(57,2,2),(21,7,7),(50,3,3),(55,26,26),(58,15,15),(4,0,0),
    (35,11,11),(63,18,18),(25,0,0),(63,6,6),(14,0,0),(63,22,22),(53,16,16),(35,7,7),
    (22,10,10),(63,32,32),(48,15,15),(57,10,10),(14,4,4),(50,9,9),(43,17,17),(27,4,4),
    (43,11,11),(43,1,1),(32,6,6),(63,4,4),(63,28,28),(47,7,7),(63,20,20),(63,12,12),
    (28,6,6),(17,0,0),(63,35,35),(63,16,16),(29,0,0),(42,6,6),(63,24,24),(8,0,0),
    (57,23,23),(29,12,12),(20,3,3),(63,8,8),(40,15,15),(58,18,18),(29,2,2),(12,0,0),
    (63,26,26),(58,4,4),(32,10,10),(43,20,20),(6,0,0),(30,10,10),(47,19,19),(0,0,0),
]

def vga_to_8(v):
    return (v * 255 + 31) // 63

# Build RGBA palette (index 0 = fully transparent)
PALETTE = []
for i, (r, g, b) in enumerate(XQ_PAL_VGA):
    PALETTE.append((vga_to_8(r), vga_to_8(g), vga_to_8(b), 0 if i == 0 else 255))


def ship_to_rgba(frame: dict) -> Image.Image:
    w, h = frame["width"], frame["height"]
    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    pixels = frame["pixels"]
    for y in range(h):
        for x in range(w):
            idx = pixels[y * w + x]
            img.putpixel((x, y), PALETTE[idx])
    return img


def make_icon(ship_img: Image.Image, size: int) -> Image.Image:
    """Scale ship to fill ~80% of size×size canvas, nearest-neighbour, on transparent bg."""
    margin = max(2, size // 8)
    ship_size = size - 2 * margin
    # Round down to multiple of ship_img.width so pixels stay crisp
    scale = max(1, ship_size // ship_img.width)
    scaled_w = ship_img.width * scale
    scaled_h = ship_img.height * scale
    scaled = ship_img.resize((scaled_w, scaled_h), Image.NEAREST)

    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    off_x = (size - scaled_w) // 2
    off_y = (size - scaled_h) // 2
    canvas.paste(scaled, (off_x, off_y), scaled)
    return canvas


def main():
    sprites_path = ASSETS_DIR / "sprites.json"
    if not sprites_path.exists():
        sys.exit(f"sprites.json not found at {sprites_path}\nRun tools/decode_assets.py first.")

    with open(sprites_path) as f:
        data = json.load(f)

    # Frame 0 = ship pointing straight up - best for an icon
    frame = data["ship_frames"][0]
    ship = ship_to_rgba(frame)

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    # Standard freedesktop hicolor sizes
    for size in (16, 32, 48, 64, 128, 256):
        icon = make_icon(ship, size)
        path = OUT_DIR / f"xquest_{size}.png"
        icon.save(path)
        print(f"  wrote {path}")

    # Also write the canonical xquest.png at 256 for .desktop / AppImage
    canonical = OUT_DIR / "xquest.png"
    make_icon(ship, 256).save(canonical)
    print(f"  wrote {canonical}  (canonical 256px)")


if __name__ == "__main__":
    main()
