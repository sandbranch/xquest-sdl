# XQUEST-SDL

A faithful port of XQUEST v1.3 (DOS, 1994, Turbo Pascal) to modern Linux using C and SDL2.

## Source material

Original source lives at `../xquest/`. Key files:
- `xquest.pas` - main game loop, all gameplay logic (~3,854 lines)
- `xqvars.pas` - all global types, constants, and data structures
- `xqinit.pas` - initialization, menus, font/sprite loading (~2,183 lines)
- `xqenter.pas` - 18 enemy type definitions
- `starunit.pas` - animated 3D starfield (menu background)
- `sbunit.pas` - Sound Blaster PCM audio mixer
- `keyboard.pas`, `mouse.pas`, `joystick.pas` - input drivers
- `xquest.gfx` - all sprites (ship, enemies, objects, fonts, gates)
- `xquest.enm` - binary enemy definitions
- `xquest.snd` - raw 8-bit 11025 Hz mono PCM sound effects
- `xquest.scr` - binary high score table
- `xquest.dmo` - demo recording

## What the game is

Top-down arcade shooter. Player flies a ship (24 rotational frames) through a scrolling level, collecting crystals to open an exit gate, while avoiding/destroying up to 18 distinct enemy types and their missiles. 50 levels, 5 difficulty tiers, powerups, smart bombs, demo recording/playback.

Original resolution: 320×240 (Mode X VGA), split into a 320×217 game area and a 23-line status bar.

## Port goals

- Faithful gameplay: same physics, same AI, same level data, same collision feel
- Native Linux binary, no DOS emulation
- SDL2 for window, rendering, audio, and input
- C99 (or C11), no C++
- Keep the original 320×240 logical resolution; scale to window size at render time
- Retain the original binary asset formats where practical (gfx, enm, snd, scr, dmo)

## Architecture to build

### Core systems

| System | Original | SDL port |
|--------|----------|----------|
| Window / display | Mode X page-flip, 320×240 | SDL_Renderer with logical size 320×240, scaled texture |
| Sprites | Planar compiled bitmaps + bitmask | SDL_Texture per sprite, separate bitmask arrays for collision |
| Scrolling | Manual screen offset | Camera offset applied at render time |
| Split screen | Hardware scanline split | Render game area and HUD to separate SDL_Rect regions |
| Audio | SB DMA 8-bit 11025 Hz mono, 8 channels | SDL_mixer or SDL_AudioStream; same 8-channel mix |
| Keyboard | Custom INT 9 handler, 255-key state | SDL_KEYDOWN/KEYUP events → bool key_state[SDL_NUM_SCANCODES] |
| Mouse | INT 33h delta motion | SDL_GetRelativeMouseState |
| Joystick | Analog port $201 | SDL_Joystick / SDL_GameController |
| Timer | PZT timer, ~67 fps target | SDL_GetTicks64, fixed timestep loop |
| Config | xquest.cfg text (CRLF, `value label` lines) | Same layout, per-user data dir |
| High scores | xquest.scr binary | Same binary layout, same path |
| Demo | xquest.dmo binary | Same binary layout, config dir |

### Game object limits (from xqvars.pas)

- Enemies: max 40
- Player missiles: max 50
- Enemy missiles: max 70
- Ship rotation frames: 24
- Levels: 50
- Enemy types: 18
- Difficulty tiers: 5

### Collision detection

Original uses pixel-accurate bitmasks: 32-bit-wide rows AND'd together. Preserve this approach - bounding-box pre-check, then bitmask AND. Store masks as `uint32_t` arrays alongside textures.

### Coordinate system

Original game world is larger than the viewport; the camera scrolls to follow the ship. Keep logical coords in game-world space; apply camera offset only in the render step.

## Enemy types (xqenter.pas)

1. SuperCrystal - fast random bouncer, high value
2. Explosion - temporary death sprite
3. Grunger - basic slow enemy
4. Zippo - fast curved movement
5. Zinger - curved + fires
6. Vince - rebounds off walls
7. Hibernator - bouncing type
8. Miner - curved, lays mines
9. Meeby - high HP, follows player (2000 pts)
10. Retaliator - fires bouncing shots when hit
11. Terrier - zoom attack
12. Doinger - rapid fire
13. Snipe - precision fire
14. Tribbler - splits into smaller enemies on death
15. Buckshot - heavy fire rate
16. Cluster - explodes into projectiles on death
17. Sticktight - relentless follower
18. Repulsor - pushes player, highest value (7500 pts)

## Powerups

Shield, AimedFire, RapidFire, MultiFire, AssFire, HeavyFire, Bounce - all timer-based, defined in xqvars.pas.

## Build system

Use CMake. Target: `xquest`. Link: SDL2, SDL2_mixer (or SDL2 audio directly).

Suggested source layout:
```
src/
  main.c
  game.c / game.h        -- main loop, level logic
  entities.c / entities.h -- ship, enemies, missiles
  render.c / render.h    -- all SDL drawing
  audio.c / audio.h      -- sound mixer
  input.c / input.h      -- keyboard/mouse/joystick unified
  assets.c / assets.h    -- load gfx/enm/snd/fnt files
  collision.c / collision.h
  menu.c / menu.h
  hiscore.c / hiscore.h
  demo.c / demo.h
assets/                  -- symlink or copy from ../xquest/
```

## Implementation notes

- The original targets ~67 fps (15 ms per tick). Use a fixed-timestep loop: accumulate real elapsed time, step in 15 ms increments, render with interpolation or just snap.
- xquest.gfx uses a custom planar format. Write an asset loader that converts it to SDL_Surface/SDL_Texture on startup. Preserve the raw bitmask data for collision.
- xquest.snd contains concatenated raw PCM chunks. The enm file records offsets/lengths.
- Demo playback stores (delx, dely, buttons) per frame plus initial random seed.
  Implemented in src/demo.c: 79-byte header (seed, GameMode, two 37-byte
  PlayerInfo records) then 5-byte frames. Note the PlayerInfo field order
  differs from xquest.cfg's. Demos this port records replay exactly, but the
  original 1994 xquest.dmo will not: faithful replay needs a matching random
  number stream, and the port uses xorshift32 where Turbo Pascal used its own
  LCG.
- The status bar HUD (bottom 23 lines) shows: score, lives, level, bombs, active powerups. Render it as a fixed overlay after the game viewport.
- Starfield: 400 stars, perspective projection, used behind menus. Straightforward to re-implement with SDL_RenderDrawPoint.

## What NOT to change

- Gameplay physics and AI behaviour - stay faithful to the Pascal source
- Level data (the `probs` arrays in xqvars.pas) - copy verbatim as C arrays
- File formats for cfg/scr/dmo - keep compatibility with original files.
  Note cfg is plain CRLF text, not binary; scr and dmo are binary.
- The 320×240 logical resolution - scale at display time only
