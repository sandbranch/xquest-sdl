#pragma once
#include "assets.h"
#include "game.h"
#include "starfield.h"
#include <SDL2/SDL.h>
#include <stdbool.h>

#define RENDER_W      320
#define RENDER_H      240
#define VIEWPORT_H    217   /* game-area height (SplitScreenLine) */
#define HUD_Y         217
#define HUD_H          23

/* Window size is always a whole multiple of RENDER_W x RENDER_H, so every
   game pixel stays a square block. Matches mario-final-sdl's range. */
#define SCALE_MIN       1
#define SCALE_MAX      16

typedef struct {
    SDL_Window   *window;   /* kept for the fullscreen and resize hotkeys */
    int           scale;    /* window size as a multiple of 320x240 */
    SDL_Renderer *renderer;
    SDL_Texture  *screen;   /* 320×240 ARGB streaming texture */
    uint32_t      buf[RENDER_W * RENDER_H];
    uint32_t      pal[256]; /* pre-expanded ARGB palette, index 0 = transparent */
} Renderer;

/* Create renderer attached to window. Returns 0 on success. */
int  renderer_init(Renderer *r, SDL_Window *win, const RGB palette[256]);

/* Largest whole multiple of 320x240 fitting in `percent` of the display's
   usable area, clamped to SCALE_MIN..SCALE_MAX. Callable before any window
   exists, as long as SDL's video subsystem is up. Use 90 for a comfortable
   default (room for panels and the title bar) and 100 as the hard ceiling
   for a scale the user asked for. */
int  renderer_fit_scale(int display, int percent);

/* Resize the window to `scale`, capped at what fits the display it is on.
   Leaves fullscreen if we were in it, and recentres. */
void renderer_set_scale(Renderer *r, int scale);

bool renderer_is_fullscreen(const Renderer *r);
void renderer_toggle_fullscreen(Renderer *r);

/* Handle the display hotkeys every event loop shares: F11 or Alt+Enter
   toggles fullscreen, Ctrl+plus / Ctrl+minus resize the window a step at a
   time. Returns true when the event was ours, so callers can skip their own
   handling instead of also reading it as "any key pressed". */
bool renderer_display_event(Renderer *r, const SDL_Event *ev);
void renderer_destroy(Renderer *r);

/* Clear the game-area rows (0..VIEWPORT_H-1) to palette index. */
void render_clear_game(Renderer *r, uint8_t idx);

/* Clear the HUD rows (HUD_Y..RENDER_H-1) to palette index. */
void render_clear_hud(Renderer *r, uint8_t idx);

/* Blit a sprite at screen (x,y), clipped to the full 320×240 buffer.
   Index 0 is transparent. */
void render_sprite(Renderer *r, const Sprite *s, int x, int y);

/* Blit a sprite at world coords, offset by camera (cam_x, cam_y).
   Clips to the game viewport (y < VIEWPORT_H). */
void render_sprite_cam(Renderer *r, const Sprite *s,
                       int world_x, int world_y, int cam_x, int cam_y);

/* Draw all stars into the game area (y < VIEWPORT_H). Call after
   render_clear_game, before any sprites. */
void render_starfield(Renderer *r, const Starfield *sf);

/* Upload framebuffer and present. */
void renderer_present(Renderer *r);

/* Draw the world border (5-line 3D pipes, gate sprites, corners, enemy gates).
   Coordinates are world-space; camera offset is taken from g. */
void render_world_border(Renderer *r, const Assets *a, const GameState *g);

/* Draw all active map objects (crystals, mines, smart bombs) with camera offset. */
void render_objects(Renderer *r, const Assets *a, const GameState *g);

/* Blit text at screen (x,y) using the font glyphs. Advances 8px per char.
   Palette index 14 (font background) is treated as transparent. */
void render_text(Renderer *r, const Assets *a, int x, int y, const char *str);

/* Draw the full status-bar HUD (score, lives, bombs, crystals). */
void render_hud(Renderer *r, const Assets *a, const GameState *g);

/* Draw all active player missiles with camera offset. */
void render_missiles(Renderer *r, const Assets *a, const GameState *g);

/* Draw all active enemies with camera offset.
   Gate animation frame is derived from g->enemy_entering_*. */
void render_enemies(Renderer *r, const Assets *a, const GameState *g);

/* Draw the enemy gate sprites, using the active countdown to pick the
   correct animation frame (replaces the static frame-0 call in
   render_world_border). */
void render_enemy_gates(Renderer *r, const Assets *a, const GameState *g);

/* Draw all active enemy missiles with camera offset. */
void render_emissiles(Renderer *r, const Assets *a, const GameState *g);

/* Blit a sprite at screen (x,y), each row drawn twice (2× vertical scale). */
void render_sprite_2xv(Renderer *r, const Sprite *s, int x, int y);

/* --- Comix font (xquest2.fnt) rendering --- */

/* Pixel-width of a string rendered in the comix font.
   Unmapped characters (e.g. unloaded glyphs) contribute 4 px. */
int comix_text_width(const Assets *a, const char *str);

/* Render str at (x,y) using the comix font.
   Pixel value 0 is transparent; values 1 and 2 both render as palette[color_idx].
   x advances by each glyph's own width plus 1 px inter-glyph spacing. */
void render_comix_text(Renderer *r, const Assets *a, int x, int y,
                       uint8_t color_idx, const char *str);

/* Smart-bomb flash overlay. level 1-11: tints the game viewport pixels with
   the SmartBombPal colour for that level (VGA palette-flash equivalent). */
void render_flash_overlay(Renderer *r, int level);
