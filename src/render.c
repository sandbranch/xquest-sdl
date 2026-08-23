#include "render.h"
#include "starfield.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int renderer_init(Renderer *r, SDL_Window *win, const RGB palette[256]) {
    r->renderer = SDL_CreateRenderer(win, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!r->renderer) {
        fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return -1;
    }
    SDL_RenderSetLogicalSize(r->renderer, 320, 240);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    r->screen = SDL_CreateTexture(r->renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        320, 240);
    if (!r->screen) {
        fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        SDL_DestroyRenderer(r->renderer);
        return -1;
    }

    /* Build ARGB lookup. Index 0 is the "transparent" key colour - rendered
       as black (0xFF000000) on the background, but masked out when blitting
       sprites on top of other content.  We record it as opaque here; the
       blit function handles the skip. */
    for (int i = 0; i < 256; i++) {
        uint8_t rv = palette[i].r;
        uint8_t gv = palette[i].g;
        uint8_t bv = palette[i].b;
        r->pal[i] = (0xFFu << 24) | ((uint32_t)rv << 16)
                  | ((uint32_t)gv << 8) | bv;
    }

    memset(r->buf, 0, sizeof(r->buf));
    return 0;
}

void renderer_destroy(Renderer *r) {
    if (r->screen)   SDL_DestroyTexture(r->screen);
    if (r->renderer) SDL_DestroyRenderer(r->renderer);
    r->screen   = NULL;
    r->renderer = NULL;
}

void render_clear_game(Renderer *r, uint8_t idx) {
    uint32_t c = r->pal[idx];
    for (int i = 0; i < RENDER_W * VIEWPORT_H; i++) r->buf[i] = c;
}

void render_clear_hud(Renderer *r, uint8_t idx) {
    uint32_t c = r->pal[idx];
    for (int i = RENDER_W * HUD_Y; i < RENDER_W * RENDER_H; i++) r->buf[i] = c;
}

void render_sprite(Renderer *r, const Sprite *s, int x, int y) {
    for (int row = 0; row < s->h; row++) {
        int dy = y + row;
        if (dy < 0 || dy >= RENDER_H) continue;
        for (int col = 0; col < s->w; col++) {
            int dx = x + col;
            if (dx < 0 || dx >= RENDER_W) continue;
            uint8_t px = s->px[row * s->w + col];
            if (px == 0) continue;
            r->buf[dy * RENDER_W + dx] = r->pal[px];
        }
    }
}

void render_sprite_cam(Renderer *r, const Sprite *s,
                       int world_x, int world_y, int cam_x, int cam_y) {
    int sx = world_x - cam_x;
    int sy = world_y - cam_y;
    for (int row = 0; row < s->h; row++) {
        int dy = sy + row;
        if (dy < 0 || dy >= VIEWPORT_H) continue;
        for (int col = 0; col < s->w; col++) {
            int dx = sx + col;
            if (dx < 0 || dx >= RENDER_W) continue;
            uint8_t px = s->px[row * s->w + col];
            if (px == 0) continue;
            r->buf[dy * RENDER_W + dx] = r->pal[px];
        }
    }
}

void render_starfield(Renderer *r, const Starfield *sf) {
    for (int i = 0; i < STAR_COUNT; i++) {
        const Star *st = &sf->stars[i];
        if (st->c <= 0) continue;
        if (st->xz < 0 || st->xz >= RENDER_W) continue;
        if (st->yz < 0 || st->yz >= VIEWPORT_H) continue;
        r->buf[st->yz * RENDER_W + st->xz] = r->pal[st->c];
    }
}

/* Horizontal world-space line at world_y from world x1 to x2 (inclusive),
   camera-offset, clipped to the game viewport. */
static void render_hline_cam(Renderer *r, int x1, int x2, int world_y,
                              int cam_x, int cam_y, uint32_t color) {
    int sy = world_y - cam_y;
    if (sy < 0 || sy >= VIEWPORT_H) return;
    int sx1 = x1 - cam_x;
    int sx2 = x2 - cam_x;
    if (sx1 < 0) sx1 = 0;
    if (sx2 >= RENDER_W) sx2 = RENDER_W - 1;
    if (sx1 > sx2) return;
    uint32_t *row = r->buf + sy * RENDER_W;
    for (int x = sx1; x <= sx2; x++) row[x] = color;
}

/* Vertical world-space line at world_x from world y1 to y2 (inclusive). */
static void render_vline_cam(Renderer *r, int y1, int y2, int world_x,
                              int cam_x, int cam_y, uint32_t color) {
    int sx = world_x - cam_x;
    if (sx < 0 || sx >= RENDER_W) return;
    int sy1 = y1 - cam_y;
    int sy2 = y2 - cam_y;
    if (sy1 < 0) sy1 = 0;
    if (sy2 >= VIEWPORT_H) sy2 = VIEWPORT_H - 1;
    if (sy1 > sy2) return;
    for (int y = sy1; y <= sy2; y++) r->buf[y * RENDER_W + sx] = color;
}

void render_world_border(Renderer *r, const Assets *a, const GameState *g) {
    /* bcolor[1..5] = {10,15,25,15,10} - 3D pipe gradient (xqvars.pas) */
    static const uint8_t bcolor[6] = {0, 10, 15, 25, 15, 10};
    int cx = g->cam_x, cy = g->cam_y;

    for (int i = 1; i <= 5; i++) {
        uint32_t c = r->pal[bcolor[i]];
        /* Top border: two segments around the gate opening */
        render_hline_cam(r, 10, g->gate_left_x, 10 - i, cx, cy, c);
        render_hline_cam(r, g->gate_right_x + a->gate_right.w, WORLD_W - 10, 10 - i, cx, cy, c);
        /* Bottom border */
        render_hline_cam(r, 10, WORLD_W - 10, WORLD_H - 5 - i, cx, cy, c);
        /* Left border */
        render_vline_cam(r, 10, WORLD_H - 9, 4 + i, cx, cy, c);
        /* Right border */
        render_vline_cam(r, 10, WORLD_H - 9, WORLD_W - 11 + i, cx, cy, c);
    }

    /* Gate sprites at world y=0 */
    render_sprite_cam(r, &a->gate_left,  g->gate_left_x,  0, cx, cy);
    render_sprite_cam(r, &a->gate_right, g->gate_right_x, 0, cx, cy);

    /* Red blocker line while gate is closed (palette index 54 = bright red) */
    if (!g->gate_open)
        render_hline_cam(r, g->gate_left_x + a->gate_left.w,
                         g->gate_right_x - 1, 7, cx, cy, r->pal[54]);

    /* Corner sprites: tl=0 (0,0), tr=1 (382,0), br=2 (382,310), bl=3 (0,310) */
    render_sprite_cam(r, &a->corner[0], 0,            0,             cx, cy);
    render_sprite_cam(r, &a->corner[1], WORLD_W - 10, 0,             cx, cy);
    render_sprite_cam(r, &a->corner[2], WORLD_W - 10, WORLD_H - 10,  cx, cy);
    render_sprite_cam(r, &a->corner[3], 0,            WORLD_H - 10,  cx, cy);

    /* Enemy gate sprites rendered by render_enemy_gates() instead */
}

void render_objects(Renderer *r, const Assets *a, const GameState *g) {
    for (int i = 0; i < g->num_objects; i++) {
        const Object *o = &g->objects[i];
        if (!o->active) continue;
        const Sprite *sp;
        switch (o->type) {
            case OBJ_CRYSTAL:    sp = &a->crystal;    break;
            case OBJ_MINE:       sp = &a->mine;       break;
            case OBJ_SMART:      sp = &a->smart_bomb; break;
            case OBJ_ENEMY_MINE: sp = &a->enemy_mine; break;
            default:          continue;
        }
        render_sprite_cam(r, sp, o->x, o->y, g->cam_x, g->cam_y);
    }
}

void render_text(Renderer *r, const Assets *a, int x, int y, const char *str) {
    for (; *str; str++, x += 8) {
        uint8_t gi = g_fontmap[(unsigned char)*str];
        if (gi == 0) continue;
        const Sprite *s = &a->font[gi - 1];
        for (int row = 0; row < s->h; row++) {
            int dy = y + row;
            if (dy < 0 || dy >= RENDER_H) continue;
            for (int col = 0; col < s->w; col++) {
                int dx = x + col;
                if (dx < 0 || dx >= RENDER_W) continue;
                uint8_t px = s->px[row * s->w + col];
                if (px == 14) continue;   /* palette index 14 = font background */
                r->buf[dy * RENDER_W + dx] = r->pal[px];
            }
        }
    }
}

void render_hud(Renderer *r, const Assets *a, const GameState *g) {
    char s[32];

    /* Score: 8 chars right-justified at (10, HUD_Y+5), matching str(score:8,s) */
    snprintf(s, sizeof(s), "%8ld", g->score);
    render_text(r, a, 10, HUD_Y + 5, s);

    /* Lives: text (2 spaces + count) then ship icon on top of the spaces */
    snprintf(s, sizeof(s), "  %d", g->lives - 1);
    render_text(r,   a, 185, HUD_Y + 5, s);
    render_sprite(r, &a->ship_icon,      185, HUD_Y + 6);

    /* Smart bombs */
    snprintf(s, sizeof(s), "  %d", g->bombs);
    render_text(r,   a, 230, HUD_Y + 5, s);
    render_sprite(r, &a->smart_bomb_hud, 230, HUD_Y + 6);

    /* Crystals collected (total minus remaining, counts up toward level quota) */
    int crystals_collected = g_levels[g->level - 1].numcryst - g->num_crystals;
    snprintf(s, sizeof(s), "  %d", crystals_collected);
    render_text(r,   a, 275, HUD_Y + 5, s);
    render_sprite(r, &a->crystal_hud,    275, HUD_Y + 6);

    /* Powerup icons: 7 slots starting at x=80, 16px spacing.
       Blink when timer < 198 ticks: show during upper half of 22-tick cycle. */
    for (int pu = 0; pu < PU_COUNT; pu++) {
        int t = g->powerup_timer[pu];
        if (t <= 0) continue;
        bool show = (t >= 198) || ((t % 22) >= 11);
        if (show && a->powerup_hud[pu].px)
            render_sprite(r, &a->powerup_hud[pu], 80 + pu * 16, HUD_Y + 4);
    }
}

void render_missiles(Renderer *r, const Assets *a, const GameState *g) {
    for (int i = 0; i < g->num_missiles; i++) {
        const Missile *m = &g->missiles[i];
        if (!m->active) continue;
        render_sprite_cam(r, &a->player_missile, m->x, m->y, g->cam_x, g->cam_y);
    }
}

void render_enemy_gates(Renderer *r, const Assets *a, const GameState *g) {
    int cx = g->cam_x, cy = g->cam_y;
    int lf = 0, rf = 0;
    if (g->enemy_entering_left  > 0)
        lf = (g->enemy_entering_left  >> 3) % 5 + 1;
    if (g->enemy_entering_right > 0)
        rf = (g->enemy_entering_right >> 3) % 5 + 1;
    render_sprite_cam(r, &a->enemy_gate_left[lf],  0,            WORLD_H / 2 - 10, cx, cy);
    render_sprite_cam(r, &a->enemy_gate_right[rf], WORLD_W - 20, WORLD_H / 2 - 10, cx, cy);
}

void render_enemies(Renderer *r, const Assets *a, const GameState *g) {
    for (int i = 0; i < g->num_enemies; i++) {
        const Enemy *e = &g->enemies[i];
        if (!e->active) continue;
        int fidx = e->frame >> 8;
        const Sprite *sp = &a->enemy[e->type_idx][fidx];
        if (!sp->px) continue;
        render_sprite_cam(r, sp, e->x, e->y, g->cam_x, g->cam_y);
    }
}

void render_emissiles(Renderer *r, const Assets *a, const GameState *g) {
    for (int i = 0; i < g->num_emissiles; i++) {
        const EnemyMissile *em = &g->emissiles[i];
        if (!em->active) continue;
        render_sprite_cam(r, &a->enemy_missile[em->firetype - 1],
                          em->x, em->y, g->cam_x, g->cam_y);
    }
}

int comix_text_width(const Assets *a, const char *str) {
    int w = 0;
    for (const char *p = str; *p; p++) {
        unsigned char c = (unsigned char)*p;
        int gw = (c < 128 && a->comix_font[c].px) ? a->comix_font[c].w : 4;
        if (w > 0) w += 1;   /* 1 px inter-glyph gap */
        w += gw;
    }
    return w;
}

void render_comix_text(Renderer *r, const Assets *a, int x, int y,
                       uint8_t color_idx, const char *str) {
    bool first = true;
    for (const char *p = str; *p; p++) {
        unsigned char c = (unsigned char)*p;
        if (!first) x += 1;
        first = false;
        if (c >= 128 || !a->comix_font[c].px) { x += 4; continue; }
        const Sprite *s = &a->comix_font[c];
        for (int row = 0; row < s->h; row++) {
            int dy = y + row;
            if (dy < 0 || dy >= RENDER_H) continue;
            for (int col = 0; col < s->w; col++) {
                int dx = x + col;
                if (dx < 0 || dx >= RENDER_W) continue;
                uint8_t v = s->px[row * s->w + col];
                /* XPutFontPBM: "adds value of Color to every pixel".
                   value 1 → pal[color_idx+1] (main stroke)
                   value 2 → pal[color_idx+2] (shadow/anti-alias) */
                if (v) r->buf[dy * RENDER_W + dx] = r->pal[color_idx + v];
            }
        }
        x += s->w;
    }
}

void render_sprite_2xv(Renderer *r, const Sprite *s, int x, int y) {
    for (int row = 0; row < s->h; row++) {
        for (int r2 = 0; r2 < 2; r2++) {
            int dy = y + row * 2 + r2;
            if (dy < 0 || dy >= RENDER_H) continue;
            for (int col = 0; col < s->w; col++) {
                int dx = x + col;
                if (dx < 0 || dx >= RENDER_W) continue;
                uint8_t px = s->px[row * s->w + col];
                if (px == 0) continue;
                r->buf[dy * RENDER_W + dx] = r->pal[px];
            }
        }
    }
}

/* SmartBombPal from xqvars.pas (VGA 6-bit * 4 → 8-bit), indices 1-11 */
static const uint8_t sbpal_r[12] = {0,  0, 32, 56, 80,104,128,152,176,200,224,252};
static const uint8_t sbpal_g[12] = {0,  0,  0,  0, 16, 48, 80,112,144,176,208,252};
static const uint8_t sbpal_b[12] = {0,  0,  0,  0,  0, 40, 80,120,160,200,200,252};

void render_flash_overlay(Renderer *r, int level) {
    if (level < 1 || level > 11) return;
    uint8_t rv = sbpal_r[level];
    uint8_t gv = sbpal_g[level];
    uint8_t bv = sbpal_b[level];
    /* Blend flash colour over game viewport pixels (not HUD) */
    for (int y = 0; y < VIEWPORT_H; y++) {
        uint32_t *row = r->buf + y * RENDER_W;
        for (int x = 0; x < RENDER_W; x++) {
            uint32_t px = row[x];
            uint8_t sr = (px >> 16) & 0xFF;
            uint8_t sg = (px >>  8) & 0xFF;
            uint8_t sb =  px        & 0xFF;
            /* additive-clamp: raises dark pixels toward flash colour */
            uint8_t nr = sr + rv > 255 ? 255 : sr + rv;
            uint8_t ng = sg + gv > 255 ? 255 : sg + gv;
            uint8_t nb = sb + bv > 255 ? 255 : sb + bv;
            row[x] = 0xFF000000 | ((uint32_t)nr << 16) | ((uint32_t)ng << 8) | nb;
        }
    }
}

void renderer_present(Renderer *r) {
    void   *pixels;
    int     pitch;
    SDL_LockTexture(r->screen, NULL, &pixels, &pitch);
    /* pitch may differ from 320*4 if SDL adds padding; copy row-by-row */
    for (int row = 0; row < 240; row++) {
        memcpy((uint8_t *)pixels + row * pitch,
               r->buf + row * 320,
               320 * sizeof(uint32_t));
    }
    SDL_UnlockTexture(r->screen);
    SDL_RenderClear(r->renderer);
    SDL_RenderCopy(r->renderer, r->screen, NULL, NULL);
    SDL_RenderPresent(r->renderer);
}
