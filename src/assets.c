#include "assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ASCII (0..128) → glyph index 1..40 in a->font[] (0 = unmapped). */
const uint8_t g_fontmap[129] = {
    /* 0 */  0,
    /* 1-31: control chars */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 32: space */  39,
    /* 33-42: !"#$%&'()*  */ 0,0,0,0,0,0,0,0,0,0,
    /* 43: + */  37,
    /* 44-47: ,-./  */  0,0,0,0,
    /* 48-57: 0-9  */  1,2,3,4,5,6,7,8,9,10,
    /* 58: :  */  38,
    /* 59-62: ;<=>  */  0,0,0,0,
    /* 63: ?  */  40,
    /* 64: @  */  0,
    /* 65-90: A-Z  */  11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,
    /* 91-128  */  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* Number of animation frames per enemy kind (numframes+1 from xqenter.pas).
   Indexed 0..18, matching enemy_kinds in gamedata.json. */
const int enemy_frame_counts[ENEMY_KINDS] = {
    6, /* 0  SuperCrystal  */
    6, /* 1  Explosion     */
    4, /* 2  Grunger       */
    4, /* 3  Zippo         */
    4, /* 4  Zinger        */
    4, /* 5  Vince         */
    1, /* 6  Hibernator    */
    4, /* 7  Miner         */
    6, /* 8  Meeby         */
    4, /* 9  Retaliator    */
    4, /* 10 Terrier       */
    4, /* 11 Doinger       */
    4, /* 12 Snipe         */
    4, /* 13 Tribbler      */
    4, /* 14 Tribble       */
    4, /* 15 Buckshot      */
    4, /* 16 Cluster       */
    4, /* 17 Sticktight    */
    6, /* 18 Repulsor      */
};

/* VGA palette: 256 entries, 6-bit values (0-63). */
static const uint8_t xq_pal_vga[256][3] = {
    {0,0,0},{2,2,2},{4,4,4},{6,6,6},{8,8,8},{10,10,10},{12,12,12},{14,14,14},
    {16,16,16},{18,18,18},{20,20,20},{22,22,22},{24,24,24},{26,26,26},{28,28,28},{30,30,30},
    {32,32,32},{34,34,34},{36,36,36},{38,38,38},{40,40,40},{42,42,42},{44,44,44},{46,46,46},
    {48,48,48},{50,50,50},{52,52,52},{54,54,54},{56,56,56},{58,58,58},{60,60,60},{63,63,63},
    {0,0,6},{0,0,14},{0,0,22},{0,0,30},{0,0,38},{0,0,46},{0,0,54},{0,0,63},
    {6,6,63},{14,14,63},{22,22,63},{30,30,63},{38,38,63},{46,46,63},{54,54,63},{6,0,0},
    {14,0,0},{22,0,0},{30,0,0},{38,0,0},{46,0,0},{54,0,0},{63,0,0},{63,6,6},
    {63,14,14},{63,22,22},{63,30,30},{63,38,38},{63,46,46},{63,54,54},{0,6,0},{0,14,0},
    {0,22,0},{0,30,0},{0,38,0},{0,46,0},{0,54,0},{0,63,0},{6,63,6},{14,63,14},
    {22,63,22},{30,63,30},{38,63,38},{46,63,46},{54,63,54},{6,0,6},{14,0,14},{22,0,22},
    {30,0,30},{38,0,38},{46,0,46},{54,0,54},{63,0,63},{63,6,63},{63,14,63},{63,22,63},
    {63,30,63},{63,38,63},{63,46,63},{63,54,63},{6,6,0},{14,14,0},{22,22,0},{30,30,0},
    {38,38,0},{46,46,0},{54,54,0},{63,63,0},{63,63,6},{63,63,14},{63,63,22},{63,63,30},
    {63,63,38},{63,63,46},{63,63,54},{0,6,6},{0,14,14},{0,22,22},{0,30,30},{0,38,38},
    {0,46,46},{0,54,54},{0,63,63},{6,63,63},{14,63,63},{22,63,63},{30,63,63},{38,63,63},
    {46,63,63},{54,63,63},{6,6,2},{14,14,6},{22,22,10},{30,30,14},{38,38,18},{46,46,22},
    {54,54,26},{63,63,30},{63,63,34},{63,63,38},{63,63,42},{63,63,46},{63,63,50},{63,63,54},
    {63,63,58},{2,6,6},{6,14,14},{10,22,22},{14,30,30},{18,38,38},{22,46,46},{26,54,54},
    {30,63,63},{34,63,63},{38,63,63},{42,63,63},{46,63,63},{50,63,63},{54,63,63},{58,63,63},
    {6,2,6},{14,6,14},{22,10,22},{30,14,30},{38,18,38},{46,22,46},{54,26,54},{63,30,63},
    {63,34,63},{63,38,63},{63,42,63},{63,46,63},{63,50,63},{63,54,63},{63,58,63},{57,6,0},
    {51,12,0},{45,18,0},{38,25,0},{32,31,0},{26,37,0},{19,44,0},{13,50,0},{7,56,0},
    {0,63,0},{0,57,6},{0,51,12},{0,45,18},{0,39,24},{0,32,31},{0,26,37},{0,20,43},
    {0,14,49},{0,7,56},{0,0,63},{45,45,45},{23,23,23},{45,0,0},{23,0,0},{0,0,0},
    {0,0,0},{35,1,1},{46,22,22},{10,0,0},{34,15,15},{63,2,2},{21,0,0},{63,14,14},
    {63,30,30},{63,10,10},{57,2,2},{21,7,7},{50,3,3},{55,26,26},{58,15,15},{4,0,0},
    {35,11,11},{63,18,18},{25,0,0},{63,6,6},{14,0,0},{63,22,22},{53,16,16},{35,7,7},
    {22,10,10},{63,32,32},{48,15,15},{57,10,10},{14,4,4},{50,9,9},{43,17,17},{27,4,4},
    {43,11,11},{43,1,1},{32,6,6},{63,4,4},{63,28,28},{47,7,7},{63,20,20},{63,12,12},
    {28,6,6},{17,0,0},{63,35,35},{63,16,16},{29,0,0},{42,6,6},{63,24,24},{8,0,0},
    {57,23,23},{29,12,12},{20,3,3},{63,8,8},{40,15,15},{58,18,18},{29,2,2},{12,0,0},
    {63,26,26},{58,4,4},{32,10,10},{43,20,20},{6,0,0},{30,10,10},{47,19,19},{0,0,0},
};

static uint8_t vga_to_8(uint8_t v) {
    return (uint8_t)((v * 255u + 31u) / 63u);
}

static int read_u16le(FILE *f, uint16_t *out) {
    uint8_t buf[2];
    if (fread(buf, 1, 2, f) != 2) return -1;
    *out = (uint16_t)(buf[0] | (buf[1] << 8));
    return 0;
}

/* Load one sprite: uint16-le w, uint16-le h, then stride*h bytes on disk
   where stride = ceil(w/4)*4.  We store only the w pixel columns per row. */
static int load_sprite(FILE *f, Sprite *s) {
    uint16_t w, h;
    if (read_u16le(f, &w) || read_u16le(f, &h)) return -1;
    int stride    = ((int)w + 3) & ~3;
    int pad_bytes = stride - (int)w;
    size_t px_n   = (size_t)w * h;
    s->w  = w;
    s->h  = h;
    s->px = malloc(px_n ? px_n : 1);
    if (!s->px) return -1;
    if (pad_bytes == 0) {
        if (px_n && fread(s->px, 1, px_n, f) != px_n) {
            free(s->px); s->px = NULL; return -1;
        }
    } else {
        uint8_t pad[3]; /* pad_bytes is at most 3 */
        for (int r = 0; r < (int)h; r++) {
            if (fread(s->px + r * w, 1, w, f) != (size_t)w ||
                fread(pad, 1, pad_bytes, f) != (size_t)pad_bytes) {
                free(s->px); s->px = NULL; return -1;
            }
        }
    }
    s->mask = calloc(h, sizeof(uint32_t));
    if (!s->mask) { free(s->px); s->px = NULL; return -1; }
    for (int r = 0; r < (int)h; r++) {
        uint32_t row = 0;
        for (int c = 0; c < (int)w && c < 32; c++) {
            if (s->px[r * w + c] != 0)
                row |= (1u << (31 - c));
        }
        s->mask[r] = row;
    }
    return 0;
}

/* Load xquest2.fnt.  Each glyph: [char_code:u8][w:u16le][h:u16le][w*h bytes].
   Pixel values: 0=transparent, 1/2=foreground (colour chosen at render time).
   Unused slots in font[128] keep px=NULL. */
static int load_comix_font(const char *path, Sprite font[128]) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }
    uint8_t  code;
    uint16_t w, h;
    while (fread(&code, 1, 1, f) == 1) {
        if (read_u16le(f, &w) || read_u16le(f, &h)) break;
        size_t sz = (size_t)w * h;
        if (code < 128) {
            Sprite *s = &font[code];
            s->w    = w;
            s->h    = h;
            s->mask = NULL;
            s->px   = malloc(sz ? sz : 1);
            if (!s->px) { fclose(f); return -1; }
            if (sz && fread(s->px, 1, sz, f) != sz) {
                free(s->px); s->px = NULL; fclose(f); return -1;
            }
        } else {
            /* skip glyph outside ASCII range */
            fseek(f, (long)sz, SEEK_CUR);
        }
    }
    fclose(f);
    return 0;
}

/* Load a pre-converted .raw sprite: [width:u16le][height:u16le][w*h palette bytes].
   Generated from the original VGA-planar .pbm files by tools/convert_pbm.py. */
static int load_raw_sprite(const char *path, Sprite *s) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }
    uint16_t w, h;
    if (read_u16le(f, &w) || read_u16le(f, &h)) { fclose(f); return -1; }
    size_t sz = (size_t)w * h;
    s->w = w;  s->h = h;  s->mask = NULL;
    s->px = malloc(sz ? sz : 1);
    if (!s->px) { fclose(f); return -1; }
    if (sz && fread(s->px, 1, sz, f) != sz) {
        free(s->px); s->px = NULL; fclose(f); return -1;
    }
    fclose(f);
    return 0;
}

static void free_sprite(Sprite *s) {
    free(s->px);
    s->px = NULL;
    free(s->mask);
    s->mask = NULL;
}

static int load_n(FILE *f, Sprite *arr, int count, const char *label) {
    for (int i = 0; i < count; i++) {
        if (load_sprite(f, &arr[i]) != 0) {
            fprintf(stderr, "assets: failed reading %s[%d]\n", label, i);
            return -1;
        }
    }
    return 0;
}

int assets_load(Assets *a, const char *dir) {
    memset(a, 0, sizeof(*a));

    for (int i = 0; i < PALETTE_SIZE; i++) {
        a->palette[i].r = vga_to_8(xq_pal_vga[i][0]);
        a->palette[i].g = vga_to_8(xq_pal_vga[i][1]);
        a->palette[i].b = vga_to_8(xq_pal_vga[i][2]);
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/xquest.gfx", dir);
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); return -1; }

    /* --- xquest.gfx layout (decode_assets.py decode_gfx) --- */

    /* 24 ship rotation frames */
    if (load_n(f, a->ship, SHIP_FRAMES, "ship") != 0) goto fail;

    /* Player missile */
    if (load_sprite(f, &a->player_missile) != 0) {
        fprintf(stderr, "assets: failed reading player_missile\n"); goto fail;
    }

    /* Collectibles: crystal, mine, smart_bomb */
    if (load_sprite(f, &a->crystal)    != 0) { fprintf(stderr, "assets: crystal\n");    goto fail; }
    if (load_sprite(f, &a->mine)       != 0) { fprintf(stderr, "assets: mine\n");       goto fail; }
    if (load_sprite(f, &a->smart_bomb) != 0) { fprintf(stderr, "assets: smart_bomb\n"); goto fail; }

    /* Enemy mine */
    if (load_sprite(f, &a->enemy_mine) != 0) { fprintf(stderr, "assets: enemy_mine\n"); goto fail; }

    /* Enemy animation frames: numframes+1 per kind */
    for (int k = 0; k < ENEMY_KINDS; k++) {
        int nf = enemy_frame_counts[k];
        for (int fr = 0; fr < nf; fr++) {
            if (load_sprite(f, &a->enemy[k][fr]) != 0) {
                fprintf(stderr, "assets: enemy[%d][%d]\n", k, fr);
                goto fail;
            }
        }
    }

    /* 6 enemy missile sprites */
    if (load_n(f, a->enemy_missile, MISSILE_KINDS, "enemy_missile") != 0) goto fail;

    /* HUD: ship icon, smart bomb, crystal */
    if (load_sprite(f, &a->ship_icon)      != 0) { fprintf(stderr, "assets: ship_icon\n");      goto fail; }
    if (load_sprite(f, &a->smart_bomb_hud) != 0) { fprintf(stderr, "assets: smart_bomb_hud\n"); goto fail; }
    if (load_sprite(f, &a->crystal_hud)    != 0) { fprintf(stderr, "assets: crystal_hud\n");    goto fail; }

    /* 7 powerup HUD icons */
    if (load_n(f, a->powerup_hud, POWERUP_KINDS, "powerup_hud") != 0) goto fail;

    /* Exit gate pair */
    if (load_sprite(f, &a->gate_left)  != 0) { fprintf(stderr, "assets: gate_left\n");  goto fail; }
    if (load_sprite(f, &a->gate_right) != 0) { fprintf(stderr, "assets: gate_right\n"); goto fail; }

    /* Border corners: tl, tr, br, bl */
    if (load_n(f, a->corner, CORNER_COUNT, "corner") != 0) goto fail;

    /* Enemy gate animation: 6 frames each side */
    if (load_n(f, a->enemy_gate_left,  GATE_FRAMES, "enemy_gate_left")  != 0) goto fail;
    if (load_n(f, a->enemy_gate_right, GATE_FRAMES, "enemy_gate_right") != 0) goto fail;

    /* Attractor */
    if (load_sprite(f, &a->attractor) != 0) { fprintf(stderr, "assets: attractor\n"); goto fail; }

    /* Small HUD digit glyphs 0-9 */
    if (load_n(f, a->digit, FONT_DIGITS, "digit") != 0) goto fail;

    fclose(f);

    /* xquest.fnt: 40 glyph sprites (same per-sprite header format as gfx) */
    snprintf(path, sizeof(path), "%s/xquest.fnt", dir);
    {
        FILE *ff = fopen(path, "rb");
        if (!ff) { perror(path); assets_free(a); return -1; }
        int rc = load_n(ff, a->font, FONT_GLYPH_COUNT, "font");
        fclose(ff);
        if (rc != 0) { assets_free(a); return -1; }
    }
    /* xquest2.fnt — comix menu font; non-fatal if missing */
    snprintf(path, sizeof(path), "%s/xquest2.fnt", dir);
    if (load_comix_font(path, a->comix_font) != 0)
        fprintf(stderr, "assets: warning: xquest2.fnt not loaded\n");

    /* startpic.raw — menu banner (pre-converted from startpic.pbm by tools/convert_pbm.py) */
    snprintf(path, sizeof(path), "%s/startpic.raw", dir);
    if (load_raw_sprite(path, &a->startpic_pbm) != 0)
        fprintf(stderr, "assets: warning: startpic.raw not loaded (run tools/convert_pbm.py)\n");

    return 0;

fail:
    fclose(f);
    assets_free(a);
    return -1;
}

void assets_free(Assets *a) {
    for (int i = 0; i < SHIP_FRAMES;  i++) free_sprite(&a->ship[i]);
    free_sprite(&a->player_missile);
    free_sprite(&a->crystal);
    free_sprite(&a->mine);
    free_sprite(&a->smart_bomb);
    free_sprite(&a->enemy_mine);
    for (int k = 0; k < ENEMY_KINDS; k++)
        for (int fr = 0; fr < ENEMY_MAX_FRAMES; fr++)
            free_sprite(&a->enemy[k][fr]);
    for (int i = 0; i < MISSILE_KINDS;  i++) free_sprite(&a->enemy_missile[i]);
    free_sprite(&a->ship_icon);
    free_sprite(&a->smart_bomb_hud);
    free_sprite(&a->crystal_hud);
    for (int i = 0; i < POWERUP_KINDS;  i++) free_sprite(&a->powerup_hud[i]);
    free_sprite(&a->gate_left);
    free_sprite(&a->gate_right);
    for (int i = 0; i < CORNER_COUNT;   i++) free_sprite(&a->corner[i]);
    for (int i = 0; i < GATE_FRAMES;    i++) free_sprite(&a->enemy_gate_left[i]);
    for (int i = 0; i < GATE_FRAMES;    i++) free_sprite(&a->enemy_gate_right[i]);
    free_sprite(&a->attractor);
    for (int i = 0; i < FONT_DIGITS;    i++) free_sprite(&a->digit[i]);
    for (int i = 0; i < FONT_GLYPH_COUNT; i++) free_sprite(&a->font[i]);
    for (int i = 0; i < 128; i++) free_sprite(&a->comix_font[i]);
    free_sprite(&a->startpic_pbm);
}
