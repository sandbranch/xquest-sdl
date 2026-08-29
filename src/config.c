#include "config.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Factory defaults, taken from the original distrib/xquest.cfg. */
void config_defaults(Config *cfg) {
    static const int keys_p1[CFG_NUM_KEYS] =
        { 200, 208, 203, 205, 71, 73, 79, 81, 76, 57, 28 };
    static const int keys_p2[CFG_NUM_KEYS] =
        {  72,  80,  75,  77, 71, 73, 79, 81, 76, 28, 57 };

    memset(cfg, 0, sizeof(*cfg));
    cfg->sound_volume = 24;
    cfg->num_players  = 1;

    for (int p = 0; p < CFG_NUM_PLAYERS; p++) {
        CfgPlayer *pl = &cfg->player[p];
        pl->h_sensitivity = 64;
        pl->v_sensitivity = 64;
        pl->difficulty    = 2;   /* Average */
        pl->input_device  = 0;   /* Mouse */
        pl->mouse_fire    = 1;
        pl->mouse_smart   = 2;
        pl->joy_fire      = 16;
        pl->joy_smart     = 32;
        memcpy(pl->keys, p == 0 ? keys_p1 : keys_p2, sizeof(pl->keys));
    }

    /* Same odd values the original shipped with; harmless, and kept so an
       untouched file round-trips identically. */
    cfg->joy_cal[0] = 8; cfg->joy_cal[1] = 30973; cfg->joy_cal[2] = 2686;
    cfg->joy_calibrated    = false;
    cfg->sound_card        = 0;
    cfg->sb_addr           = 544;
    cfg->sb_irq            = 5;
    cfg->sb_dma            = 1;
    cfg->max_sound_effects = 8;
}

/* ---- path resolution ------------------------------------------------- */

static bool file_is_writable(const char *path) {
    FILE *f = fopen(path, "r+");
    if (!f) return false;
    fclose(f);
    return true;
}

void config_path(char *buf, size_t n, const char *asset_dir) {
    const char *env = getenv("XQUEST_CONFIG_DIR");
    if (env && env[0]) {
        snprintf(buf, n, "%s/xquest.cfg", env);
        return;
    }

    /* A writable xquest.cfg beside the game data means a portable or original
       DOS-style install: use it in place, as the original did. */
    if (asset_dir && asset_dir[0]) {
        char beside[512];
        snprintf(beside, sizeof(beside), "%s/xquest.cfg", asset_dir);
        if (file_is_writable(beside)) {
            snprintf(buf, n, "%s", beside);
            return;
        }
    }

    char *pref = SDL_GetPrefPath(NULL, "xquest");
    if (pref) {
        snprintf(buf, n, "%sxquest.cfg", pref);   /* SDL_GetPrefPath ends in a separator */
        SDL_free(pref);
    } else {
        snprintf(buf, n, "xquest.cfg");
    }
}

/* ---- load ------------------------------------------------------------ */

/* Read one line, stripping the trailing CR/LF. false at EOF. */
static bool read_line(FILE *f, char *buf, size_t n) {
    if (!fgets(buf, (int)n, f)) return false;
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
        buf[--len] = '\0';
    return true;
}

/* Pascal's readln(f, i) takes the leading integer and discards the rest of the
   line, which is exactly how the trailing labels are skipped. */
static bool read_int_line(FILE *f, int *out) {
    char line[256];
    if (!read_line(f, line, sizeof(line))) return false;
    char *end;
    long v = strtol(line, &end, 10);
    if (end == line) return false;
    *out = (int)v;
    return true;
}

/* Read count integers from one line (the keys and joystick-calibration rows). */
static bool read_int_row(FILE *f, int *out, int count) {
    char line[512];
    if (!read_line(f, line, sizeof(line))) return false;
    const char *p = line;
    for (int i = 0; i < count; i++) {
        char *end;
        long v = strtol(p, &end, 10);
        if (end == p) return false;
        out[i] = (int)v;
        p = end;
    }
    return true;
}

bool config_load(Config *cfg, const char *path) {
    config_defaults(cfg);

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    Config in;
    config_defaults(&in);
    char skip[256];
    bool ok = true;

    ok = ok && read_int_line(f, &in.sound_volume);
    ok = ok && read_int_line(f, &in.num_players);

    for (int p = 0; ok && p < CFG_NUM_PLAYERS; p++) {
        CfgPlayer *pl = &in.player[p];
        ok = ok && read_line(f, skip, sizeof(skip));   /* blank */
        ok = ok && read_line(f, skip, sizeof(skip));   /* "Player One"/"Player Two" */
        ok = ok && read_int_line(f, &pl->h_sensitivity);
        ok = ok && read_int_line(f, &pl->v_sensitivity);
        ok = ok && read_int_line(f, &pl->difficulty);
        ok = ok && read_int_line(f, &pl->input_device);
        ok = ok && read_int_line(f, &pl->mouse_fire);
        ok = ok && read_int_line(f, &pl->mouse_smart);
        ok = ok && read_int_line(f, &pl->joy_fire);
        ok = ok && read_int_line(f, &pl->joy_smart);
        ok = ok && read_int_row(f, pl->keys, CFG_NUM_KEYS);
    }

    if (ok) {
        /* The tail (joystick calibration and sound hardware) is optional: a
           truncated file still yields usable player settings. */
        int cal_flag = 0;
        if (read_line(f, skip, sizeof(skip)) &&
            read_int_row(f, in.joy_cal, 8) &&
            read_int_line(f, &cal_flag)) {
            in.joy_calibrated = (cal_flag != 0);
            read_int_line(f, &in.sound_card);
            read_int_line(f, &in.sb_addr);
            read_int_line(f, &in.sb_irq);
            read_int_line(f, &in.sb_dma);
            read_int_line(f, &in.max_sound_effects);
        }
    }
    fclose(f);

    if (!ok) return false;

    /* Clamp the fields the game acts on, so a corrupt or hand-edited file
       cannot push the port out of range. The rest is passed through as-is. */
    if (in.num_players < 1 || in.num_players > 2) in.num_players = 1;
    if (in.sound_volume < 0)   in.sound_volume = 0;
    if (in.sound_volume > 128) in.sound_volume = 128;
    for (int p = 0; p < CFG_NUM_PLAYERS; p++) {
        if (in.player[p].difficulty < 0 || in.player[p].difficulty > 4)
            in.player[p].difficulty = 2;
    }

    *cfg = in;
    return true;
}

/* ---- save ------------------------------------------------------------ */

/* Turbo Pascal wrote CRLF text files; matching that keeps the file loadable by
   the DOS original and diff-clean against an untouched one. */
#define NL "\r\n"

bool config_save(const Config *cfg, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    fprintf(f, "%4d Sound Volume" NL,     cfg->sound_volume);
    fprintf(f, "%4d Number of Players" NL, cfg->num_players);

    for (int p = 0; p < CFG_NUM_PLAYERS; p++) {
        const CfgPlayer *pl = &cfg->player[p];
        fprintf(f, NL);
        fprintf(f, "%s" NL, p == 0 ? "Player One" : "Player Two");
        fprintf(f, "%4d Horizontal Input Sensitivity" NL, pl->h_sensitivity);
        fprintf(f, "%4d Vertical Input Sensitivity" NL,   pl->v_sensitivity);
        fprintf(f, "%4d Difficulty Level" NL,             pl->difficulty);
        fprintf(f, "%4d InputDevice" NL,                  pl->input_device);
        fprintf(f, "%4d Mouse Fire Button" NL,            pl->mouse_fire);
        fprintf(f, "%4d Mouse Smartbomb Button" NL,       pl->mouse_smart);
        fprintf(f, "%4d Joystick Fire Button" NL,         pl->joy_fire);
        fprintf(f, "%4d Joystick Smartbomb Button" NL,    pl->joy_smart);
        /* Pascal wrote each key followed by a space, then " Keys" - hence the
           doubled space before the label. */
        for (int k = 0; k < CFG_NUM_KEYS; k++)
            fprintf(f, "%d ", pl->keys[k]);
        fprintf(f, " Keys" NL);
    }

    fprintf(f, NL);
    fprintf(f, "%d %d %d %d %d %d %d %d Joystick calibration values" NL,
            cfg->joy_cal[0], cfg->joy_cal[1], cfg->joy_cal[2], cfg->joy_cal[3],
            cfg->joy_cal[4], cfg->joy_cal[5], cfg->joy_cal[6], cfg->joy_cal[7]);
    fprintf(f, "%4d Joystick calibrated?" NL,          cfg->joy_calibrated ? 1 : 0);
    fprintf(f, "%4d Sound card" NL,                    cfg->sound_card);
    fprintf(f, "%4d Port" NL,                          cfg->sb_addr);
    fprintf(f, "%4d IRQ" NL,                           cfg->sb_irq);
    fprintf(f, "%4d DMA" NL,                           cfg->sb_dma);
    fprintf(f, "%4d Maximum simultaneous sounds" NL,   cfg->max_sound_effects);

    /* Report write errors (full disk, read-only dir) rather than silently
       losing the settings. */
    bool ok = (ferror(f) == 0);
    if (fclose(f) != 0) ok = false;
    return ok;
}
