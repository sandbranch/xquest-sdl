#include "menu.h"
#include "render.h"
#include "starfield.h"
#include "hiscore.h"
#include "audio.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdio.h>

/* ---- shared helpers ---- */

static void fill_rect(Renderer *r, int x, int y, int w, int h, uint32_t color) {
    for (int row = y; row < y + h; row++) {
        if (row < 0 || row >= RENDER_H) continue;
        for (int col = x; col < x + w; col++) {
            if (col < 0 || col >= RENDER_W) continue;
            r->buf[row * RENDER_W + col] = color;
        }
    }
}

/* Draw a window box (2-px bright border, dark interior) */
static void draw_window(Renderer *r, int x, int y, int w, int h) {
    uint32_t border = r->pal[31];  /* bright white */
    uint32_t fill   = r->pal[1];   /* near-black interior */
    fill_rect(r, x,   y,   w,   h,   border);
    fill_rect(r, x+2, y+2, w-4, h-4, fill);
}

#define TICK_MS     15
#define NUM_ITEMS    4   /* Start Game, Difficulty, Hall of Fame, Quit */

static const char * const diff_names[5] = {
    "WIMP", "TIMID", "AVERAGE", "TRICKY", "INHUMAN"
};

static const char * const diff_names2[5] = {
    "Wimp", "Timid", "Average", "Tricky", "Inhuman"
};

/* Comix font colour offsets (matching original xqinit.pas):
 *   XPutFontPBM adds Color to each pixel value, so:
 *     pixel 1 → pal[Color+1],  pixel 2 → pal[Color+2]
 *
 *   BaseColor=186:  pal[187]={45,45,45}→light-gray, pal[188]={23,23,23}→mid-gray
 *   HighLightColor=188: pal[189] and pal[190] are animated by ColorCyclePal
 *     (grey to bright-blue pulse, matching the original XMenu animation). */
#define MENU_COLOR_SEL    188   /* HighLightColor - animated */
#define MENU_COLOR_NORMAL 186   /* BaseColor - static gray */

/* VGA→8-bit helper (matches assets.c vga_to_8) */
static uint8_t vga8(int v) { return (uint8_t)((v * 255 + 31) / 63); }

/* Original ColorCyclePal (VGA 0-63 values), grey→bright-blue→grey */
static const struct { int r, g, b; } cc_pal[13] = {
    {45,45,45},{40,40,48},{35,35,51},{30,30,54},{25,25,57},{20,20,60},
    {15,15,63},{20,20,60},{25,25,57},{30,30,54},{35,35,51},{40,40,48},{45,45,45}
};

static void color_cycle_step(Renderer *r, int *cycle) {
    int idx = (*cycle / 3) % 13;
    uint8_t rv = vga8(cc_pal[idx].r);
    uint8_t gv = vga8(cc_pal[idx].g);
    uint8_t bv = vga8(cc_pal[idx].b);
    r->pal[189] = (0xFFu << 24) | ((uint32_t)rv << 16) | ((uint32_t)gv << 8) | bv;
    r->pal[190] = (0xFFu << 24) | ((uint32_t)(rv/2) << 16)
                                | ((uint32_t)(gv/2) << 8) | (bv/2);
    if (++(*cycle) >= 13 * 3) *cycle = 0;
}

static void comix_centered(Renderer *r, const Assets *a, int y,
                            uint8_t color, const char *s) {
    int x = (RENDER_W - comix_text_width(a, s)) / 2;
    if (x < 0) x = 0;
    render_comix_text(r, a, x, y, color, s);
}

/* ---- Hall of Fame rendering ---- */

/* Render one high-score table row (rank 0-based k) at y.
   new_rank >= 0 highlights that entry; cursor_name used for active entry
   during name-entry (NULL = use stored name). */
static void render_hi_row(Renderer *r, const Assets *a,
                           const HiTable *ht, int diff, int k,
                           int new_rank, const char *cursor_name,
                           bool show_cursor) {
    int y = 67 + k * 14;
    uint8_t color = (k == new_rank) ? MENU_COLOR_SEL : MENU_COLOR_NORMAL;

    const char *name = ht->table[diff][k].name;
    if (k == new_rank && cursor_name) name = cursor_name;

    render_comix_text(r, a, 20, y, color, name);

    /* cursor during name entry */
    if (k == new_rank && show_cursor) {
        int cx = 20 + comix_text_width(a, name);
        if (cx < RENDER_W - 4) {
            uint32_t cc = r->pal[189]; /* highlight colour */
            fill_rect(r, cx, y, 2, 13, cc);
        }
    }

    char s[16];

    snprintf(s, sizeof(s), "%d", ht->table[diff][k].level);
    int lw = comix_text_width(a, s);
    render_comix_text(r, a, 305 - lw, y, color, s);

    snprintf(s, sizeof(s), "%ld", ht->table[diff][k].score);
    int sw = comix_text_width(a, s);
    render_comix_text(r, a, 270 - sw, y, color, s);
}

/* Draw the hall of fame table.  new_rank=-1 means just display; cursor_name
   and show_cursor are only used during active name-entry. */
static void render_hof_screen(Renderer *r, const Assets *a, const Starfield *sf,
                               const HiTable *ht, int diff,
                               int new_rank, const char *title,
                               const char *cursor_name, bool show_cursor) {
    memset(r->buf, 0, sizeof(r->buf));
    render_starfield(r, (Starfield *)sf);

    comix_centered(r, a, 33, MENU_COLOR_SEL, title);

    /* Column header line - y=50: title(33)+height(13)+gap(4) */
    {
        const char *hdr_name  = "Name";
        const char *hdr_score = "Score";
        const char *hdr_level = "Lv";
        render_comix_text(r, a, 20, 50, MENU_COLOR_NORMAL, hdr_name);
        int sw = comix_text_width(a, hdr_score);
        render_comix_text(r, a, 270 - sw, 50, MENU_COLOR_NORMAL, hdr_score);
        int lw = comix_text_width(a, hdr_level);
        render_comix_text(r, a, 305 - lw, 50, MENU_COLOR_NORMAL, hdr_level);
    }

    for (int k = 0; k < HI_NUM_SCORES; k++)
        render_hi_row(r, a, ht, diff, k, new_rank, cursor_name, show_cursor);
}

/* Simple SDL text-input name entry at the highlighted row.
   Returns name in `out` (max HI_NAME_MAX chars, NUL-terminated). */
static void get_player_name(const Assets *a, Renderer *r, Starfield *sf,
                             const HiTable *ht, int diff, int rank,
                             const char *title, int color_cycle_start,
                             char *out) {
    char buf[HI_NAME_MAX + 1] = "";
    int  color_cycle = color_cycle_start;
    bool show_cursor  = true;
    int  cursor_tick  = 0;

    SDL_StartTextInput();

    for (;;) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) goto done;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_RETURN ||
                    ev.key.keysym.sym == SDLK_KP_ENTER) goto done;
                if (ev.key.keysym.sym == SDLK_BACKSPACE) {
                    int len = (int)strlen(buf);
                    if (len > 0) buf[len - 1] = '\0';
                }
                if (ev.key.keysym.sym == SDLK_ESCAPE) { buf[0] = '\0'; goto done; }
            }
            if (ev.type == SDL_TEXTINPUT) {
                int len = (int)strlen(buf);
                if (len < HI_NAME_MAX) {
                    /* Only accept printable ASCII */
                    for (const char *p = ev.text.text; *p && len < HI_NAME_MAX; p++) {
                        unsigned char c = (unsigned char)*p;
                        if (c >= 32 && c < 128) buf[len++] = (char)c;
                    }
                    buf[len] = '\0';
                }
            }
        }

        /* Advance starfield and colour cycle */
        starfield_step(sf);
        color_cycle_step(r, &color_cycle);

        /* Blink cursor every 20 ticks */
        if (++cursor_tick >= 20) { cursor_tick = 0; show_cursor = !show_cursor; }

        render_hof_screen(r, a, sf, ht, diff, rank, title, buf, show_cursor);
        renderer_present(r);
        SDL_Delay(TICK_MS);
    }
done:
    SDL_StopTextInput();
    if (strlen(buf) == 0)
        strncpy(out, "Anonymous", HI_NAME_MAX);
    else
        strncpy(out, buf, HI_NAME_MAX);
    out[HI_NAME_MAX] = '\0';
}

void run_halloffame(const Assets *a, Renderer *r,
                    const HiTable *ht, int diff, int seconds) {
    Starfield sf;
    starfield_init(&sf, 0);
    int  color_cycle = 0;
    char title[48];

    snprintf(title, sizeof(title), "Hall of Fame  %s", diff_names2[diff]);

    uint32_t last_tick = SDL_GetTicks();
    uint32_t deadline  = last_tick + (uint32_t)seconds * 1000u;

    for (;;) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return;
            if (ev.type == SDL_KEYDOWN) return;
        }

        uint32_t now = SDL_GetTicks();
        if (now >= deadline) return;
        while (now - last_tick >= TICK_MS) {
            last_tick += TICK_MS;
            starfield_step(&sf);
            color_cycle_step(r, &color_cycle);
        }

        render_hof_screen(r, a, &sf, ht, diff, -1, title, NULL, false);
        renderer_present(r);
        SDL_Delay(1);
    }
}

/* ---- Main menu ---- */

/* Convert window pixel (wx,wy) to logical 320×240 coords. */
static void win_to_logical(SDL_Renderer *sdl_r, SDL_Window *win,
                            int wx, int wy, int *lx, int *ly) {
    int logic_w, logic_h, win_w, win_h;
    SDL_RenderGetLogicalSize(sdl_r, &logic_w, &logic_h);
    SDL_GetWindowSize(win, &win_w, &win_h);
    float sx    = (float)win_w / logic_w;
    float sy    = (float)win_h / logic_h;
    float scale = sx < sy ? sx : sy;
    float ox    = (win_w - logic_w  * scale) * 0.5f;
    float oy    = (win_h - logic_h  * scale) * 0.5f;
    *lx = (int)((wx - ox) / scale);
    *ly = (int)((wy - oy) / scale);
}

/* Return menu item index for logical y, or -1 if not over any item. */
static int item_at(int ly) {
    const int Y0 = 120, DY = 20;
    for (int i = 0; i < NUM_ITEMS; i++) {
        if (ly >= Y0 + i * DY && ly < Y0 + (i + 1) * DY)
            return i;
    }
    return -1;
}

int run_menu(const Assets *a, Renderer *r, SDL_Window *win,
             HiTable *ht, const char *hi_path, int *diff_io) {
    (void)hi_path;

    Starfield sf;
    starfield_init(&sf, 0);

    int sel  = 0;   /* 0=Start, 1=Difficulty, 2=Hall of Fame, 3=Quit */
    int diff = *diff_io;   /* seeded from the saved config */
    uint32_t last_tick = SDL_GetTicks();
    int color_cycle = 0;
    int result = -1;

    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);

    for (;;) {
        bool key_up    = false, key_down  = false;
        bool key_left  = false, key_right = false;
        bool key_enter = false;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) goto done;
            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE:                       goto done;
                    case SDLK_UP:    case SDLK_KP_8:       key_up    = true; break;
                    case SDLK_DOWN:  case SDLK_KP_2:       key_down  = true; break;
                    case SDLK_LEFT:  case SDLK_KP_4:       key_left  = true; break;
                    case SDLK_RIGHT: case SDLK_KP_6:       key_right = true; break;
                    case SDLK_RETURN: case SDLK_KP_ENTER:  key_enter = true; break;
                }
            }
            if (ev.type == SDL_MOUSEWHEEL) {
                if (ev.wheel.y > 0) key_up   = true;
                if (ev.wheel.y < 0) key_down = true;
            }
            if (ev.type == SDL_MOUSEMOTION) {
                int lx, ly;
                win_to_logical(r->renderer, win, ev.motion.x, ev.motion.y, &lx, &ly);
                int hovered = item_at(ly);
                if (hovered >= 0 && hovered != sel) {
                    sel = hovered;
                    audio_play(SND_MENUCLICK);
                }
            }
            if (ev.type == SDL_MOUSEBUTTONDOWN &&
                ev.button.button == SDL_BUTTON_LEFT) {
                if (sel == 1) {
                    diff = (diff + 1) % 5;  /* click cycles difficulty forward */
                    audio_play(SND_MENUCLICK);
                } else {
                    key_enter = true;
                }
            }
        }

        if (key_up)   { sel = (sel + NUM_ITEMS - 1) % NUM_ITEMS; audio_play(SND_MENUCLICK); }
        if (key_down) { sel = (sel + 1) % NUM_ITEMS;             audio_play(SND_MENUCLICK); }

        if (sel == 1) {
            if (key_left  || key_enter) diff = (diff + 4) % 5;
            if (key_right)              diff = (diff + 1) % 5;
        }

        if (key_enter) {
            if (sel == 0) { result = diff; goto done; }
            if (sel == 2) { run_halloffame(a, r, ht, diff, 120); }
            if (sel == 3) goto done;
        }

        uint32_t now = SDL_GetTicks();
        while (now - last_tick >= TICK_MS) {
            last_tick += TICK_MS;
            starfield_step(&sf);
            color_cycle_step(r, &color_cycle);
        }

        memset(r->buf, 0, sizeof(r->buf));
        render_starfield(r, &sf);

        if (a->startpic_pbm.px)
            render_sprite_2xv(r, &a->startpic_pbm, 0, 10);

        char line[48];
        const int Y0 = 120, DY = 20;

        comix_centered(r, a, Y0 + 0*DY, sel==0 ? MENU_COLOR_SEL : MENU_COLOR_NORMAL,
                       "Start Game");

        snprintf(line, sizeof(line), "Difficulty  %s", diff_names[diff]);
        comix_centered(r, a, Y0 + 1*DY, sel==1 ? MENU_COLOR_SEL : MENU_COLOR_NORMAL,
                       line);

        comix_centered(r, a, Y0 + 2*DY, sel==2 ? MENU_COLOR_SEL : MENU_COLOR_NORMAL,
                       "Hall of Fame");

        comix_centered(r, a, Y0 + 3*DY, sel==3 ? MENU_COLOR_SEL : MENU_COLOR_NORMAL,
                       "Quit");

        comix_centered(r, a, 225, MENU_COLOR_NORMAL, "Keys  Mouse");

        renderer_present(r);
        SDL_Delay(1);
    }
done:
    *diff_io = diff;   /* keep the player's choice even if they quit here */
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);
    return result;
}

/* ---- Game over ---- */

void run_game_over(const Assets *a, Renderer *r,
                   HiTable *ht, const char *hi_path,
                   int diff, long score, int level) {
    /* Show "GAME OVER" box on current framebuffer for 2 seconds */
    const int BX = 80, BY = 88, BW = 160, BH = 52;
    draw_window(r, BX, BY, BW, BH);

    int tw = comix_text_width(a, "Game Over");
    render_comix_text(r, a, BX + (BW - tw) / 2, BY + 8, MENU_COLOR_NORMAL, "Game Over");

    char line[32];
    snprintf(line, sizeof(line), "Score  %ld", score);
    int sw = comix_text_width(a, line);
    render_comix_text(r, a, BX + (BW - sw) / 2, BY + 30, MENU_COLOR_NORMAL, line);

    renderer_present(r);

    {
        uint32_t deadline = SDL_GetTicks() + 2000;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {}
        while (SDL_GetTicks() < deadline) {
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT)    goto after_box;
                if (ev.type == SDL_KEYDOWN) goto after_box;
            }
            SDL_Delay(16);
        }
    }
after_box:;

    /* Check / insert high score */
    int rank = hi_check(ht, diff, score);

    if (rank >= 0) {
        /* Insert immediately with empty name; will fill in after user types */
        hi_insert(ht, diff, rank, score, level, "");

        Starfield sf;
        starfield_init(&sf, 0);
        int color_cycle = 0;

        const char *title = (rank == 0) ? "A New High Score!" : "A Top Ten Score!";

        /* Flash title for ~1.5 seconds before prompting for name */
        uint32_t flash_end = SDL_GetTicks() + 1500;
        bool interrupted = false;
        while (SDL_GetTicks() < flash_end && !interrupted) {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT || ev.type == SDL_KEYDOWN)
                    { interrupted = true; break; }
            }
            starfield_step(&sf);
            color_cycle_step(r, &color_cycle);
            render_hof_screen(r, a, &sf, ht, diff, rank, title, "", false);
            renderer_present(r);
            SDL_Delay(TICK_MS);
        }

        /* Get player name (renders table with cursor during typing) */
        char name[HI_NAME_MAX + 1];
        get_player_name(a, r, &sf, ht, diff, rank, title, color_cycle, name);
        strncpy(ht->table[diff][rank].name, name, HI_NAME_MAX);
        ht->table[diff][rank].name[HI_NAME_MAX] = '\0';
        hi_save(ht, hi_path);

        /* Show final table for 4 seconds */
        run_halloffame(a, r, ht, diff, 4);
    } else {
        /* No high score: show hall of fame briefly */
        run_halloffame(a, r, ht, diff, 5);
    }
}
