# Changelog

All notable changes to this project will be documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added

#### Step 1 — Build system + SDL2 skeleton
- `CMakeLists.txt` — CMake 3.16+ build, C99, SDL2, `XQUEST_ASSET_DIR` cache variable
  (defaults to `../xquest`), `-Wall -Wextra -Wpedantic`
- `src/assets.h` / `src/assets.c` — loads xquest.gfx binary (uint16-le width/height
  + palette-index pixels), expands VGA 6-bit palette (0-63) to 8-bit RGB
- `src/render.h` / `src/render.c` — 320×240 ARGB streaming texture; `render_sprite`
  treats index 0 as transparent; `SDL_RenderSetLogicalSize` for integer-scaled window
- `src/main.c` — 960×720 resizable window (3× logical), 15 ms fixed tick, cycles the
  24 ship rotation frames on screen; Escape to quit

#### Project scaffolding
- `CLAUDE.md` — full port architecture reference: system mapping table (Mode X →
  SDL2), object limits, collision approach, all 18 enemy types, sprite layout,
  suggested source tree, implementation notes
- `README.md` — project overview, build requirements, asset instructions, original
  author credit
- `LICENSE` — non-commercial attribution licence inheriting the spirit of Mark
  Mackey's original XQuest shareware terms

#### Asset pipeline (`tools/decode_assets.py`)
Decoder script that reads every original binary/text asset from `../xquest/` and
writes human-readable JSON to `assets/`. Run with `python3 tools/decode_assets.py`.

Decoded assets (14 JSON files):

| File | Source | Contents |
|------|--------|----------|
| `gamedata.json` | Pascal source | Game palette (255 entries, VGA 6-bit), title screen palette, title logo palette (32-entry red ramp, indices 224–255), 19 enemy kind definitions, 6 missile kind definitions, 50 level records, per-level enemy probability tables, 5 difficulty tiers, 7 power-up durations, 25 sound names, 5 completion rank names, smart-bomb flash palette, font ASCII map, starfield parameters, music note |
| `sprites.json` | `xquest.gfx` | 24 ship rotation frames, player missile, 3 collectible sprites (crystal/mine/smart-bomb), enemy mine, 87 enemy animation frames across 19 enemy kinds, 6 enemy missile sprites, HUD icons (ship/smart-bomb/crystal/7 power-ups), gate pair, 4 border corners, 12 enemy-gate frames, attractor sprite, 10 small-font digit glyphs; all as `{width, height, pixels[]}` palette-index arrays |
| `font.json` | `xquest.fnt` | 40 in-game display font glyphs (fixed bitmap) |
| `font2.json` | `xquest2.fnt` | Full Comix display font, variable-width glyphs keyed by ASCII code |
| `sounds.json` | `xquest.snd` | 25 named PCM sound effects (8-bit, 11025 Hz, mono) with durations and base64-encoded sample data |
| `hiscores.json` | `xquest.scr` | Player's current high-score table: 5 difficulty tiers × 10 entries |
| `hiscores_defaults.json` | `distrib/xquest.scr` | Factory-shipped high-score table (all zeroes) |
| `config.json` | `xquest.cfg` | Player's saved configuration: sensitivity, difficulty, input device, key bindings, joystick calibration, sound card settings |
| `config_defaults.json` | `distrib/xquest.cfg` | Factory-shipped defaults: volume 24, difficulty Average, default PC key bindings |
| `demo.json` | `xquest.dmo` | 14 493-frame (216 s) one-player demo recording: random seed + per-frame (delx, dely, buttons) |
| `title_screen.json` | `title.pbm` | 320×240 menu background; reconstructed from two double-buffered 320×120 XLib PBM pages, converted from Mode X planar to linear palette-index layout |
| `startpic.json` | `startpic.pbm` | 320×40 menu strip image, planar→linear converted |
| `title_logo.json` | `title0.gfx` | 160×45 XQUEST title logo in raw pixel format |
| `title_logo_pbm.json` | `title0.pbm` | 160×45 title logo in XLib PBM planar format, converted to linear |

#### Key findings documented during asset extraction
- **No music.** AdLib FM synthesis code (16 instrument definitions) was written
  but commented out before v1.3 shipped. The 25 PCM effects are the complete
  audio content.
- **XLib PBM format** reverse-engineered: 1-byte `bmwidth` + 1-byte `height`
  header, followed by `bmwidth × 4 × height` bytes of Mode X planar pixel data
  (4 planes per row, `bmwidth` bytes per plane). Images wider than 255 pixels
  (i.e. 320-wide) are stored as two concatenated half-height pages.
- **`distrib/`** contains the factory-default `xquest.cfg` and `xquest.scr`
  shipped with the game, distinct from the player-modified files at the repo root.
