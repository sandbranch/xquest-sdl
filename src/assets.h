#pragma once
#include <stdint.h>
#include <stddef.h>

#define PALETTE_SIZE        256
#define SHIP_FRAMES          24
#ifndef ENEMY_KINDS
#define ENEMY_KINDS          19   /* indices 0..18, see xqenter.pas */
#endif
#define ENEMY_MAX_FRAMES      6   /* max(numframes+1) across all kinds */
#define MISSILE_KINDS         6
#define POWERUP_KINDS         7
#define FONT_DIGITS          10
#define GATE_FRAMES           6   /* enemy gate animation frames per side */
#define CORNER_COUNT          4   /* tl, tr, br, bl */
#define FONT_GLYPH_COUNT     40   /* MaxFontEntries from xqvars.pas */

typedef struct {
    uint8_t r, g, b;
} RGB;

typedef struct {
    int      w, h;
    uint8_t *px;    /* palette indices, w*h bytes, row-major */
    uint32_t *mask; /* bitmask for pixel-accurate collision, 1 uint32_t per row */
} Sprite;

/* Number of animation frames per enemy kind (numframes+1 from xqenter.pas). */
extern const int enemy_frame_counts[ENEMY_KINDS];

typedef struct {
    RGB    palette[PALETTE_SIZE];

    /* xquest.gfx — in file order */
    Sprite ship[SHIP_FRAMES];
    Sprite player_missile;
    Sprite crystal;
    Sprite mine;
    Sprite smart_bomb;
    Sprite enemy_mine;
    Sprite enemy[ENEMY_KINDS][ENEMY_MAX_FRAMES];
    Sprite enemy_missile[MISSILE_KINDS];
    Sprite ship_icon;
    Sprite smart_bomb_hud;
    Sprite crystal_hud;
    Sprite powerup_hud[POWERUP_KINDS];   /* shield, aimed, rapid, multi, ass, heavy, bounce */
    Sprite gate_left;
    Sprite gate_right;
    Sprite corner[CORNER_COUNT];          /* tl, tr, br, bl */
    Sprite enemy_gate_left[GATE_FRAMES];
    Sprite enemy_gate_right[GATE_FRAMES];
    Sprite attractor;
    Sprite digit[FONT_DIGITS];            /* small HUD digits 0-9 */
    Sprite font[FONT_GLYPH_COUNT];        /* xquest.fnt glyphs 1-40 */

    /* Menu/title assets (VGA-planar PBM files) */
    Sprite startpic_pbm;   /* startpic.pbm — 320×40 menu title banner */

    /* xquest2.fnt — variable-width comix font for menu text.
       Indexed by ASCII code 0-127; unused slots have px=NULL. */
    Sprite comix_font[128];
} Assets;

/* ASCII → font glyph index (0 = unmapped; glyph i stored in font[i-1]). */
extern const uint8_t g_fontmap[129];

/* Load all sprites from dir/xquest.gfx and expand VGA palette.
   Returns 0 on success, -1 on error (prints reason to stderr). */
int  assets_load(Assets *a, const char *dir);
void assets_free(Assets *a);
