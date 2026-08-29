#include "demo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- little-endian helpers -------------------------------------------- */

static void put_u16(uint8_t *p, int v) {
    p[0] = (uint8_t)( (unsigned)v       & 0xFF);
    p[1] = (uint8_t)(((unsigned)v >> 8) & 0xFF);
}

static int16_t get_i16(const uint8_t *p) {
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/* ---- PlayerInfoType -----------------------------------------------------

   37 bytes per player, Turbo Pascal packed record order (xqvars.pas:24):
     InputDevice (1 byte enum), then integers HInputSpeed, VInputSpeed,
     JoyFireButton, JoySmartBombButton, MouseFireButton, MouseSmartBombButton,
     DiffLevel, then KeyArray[11].

   Note this is NOT the order the settings appear in xquest.cfg, which writes
   sensitivity and difficulty before the button assignments. */

static void encode_player_info(uint8_t *out, const Config *cfg) {
    for (int p = 0; p < CFG_NUM_PLAYERS; p++) {
        const CfgPlayer *pl = &cfg->player[p];
        uint8_t *q = out + p * 37;
        *q++ = (uint8_t)pl->input_device;
        put_u16(q, pl->h_sensitivity); q += 2;
        put_u16(q, pl->v_sensitivity); q += 2;
        put_u16(q, pl->joy_fire);      q += 2;
        put_u16(q, pl->joy_smart);     q += 2;
        put_u16(q, pl->mouse_fire);    q += 2;
        put_u16(q, pl->mouse_smart);   q += 2;
        put_u16(q, pl->difficulty);    q += 2;
        for (int k = 0; k < CFG_NUM_KEYS; k++) { put_u16(q, pl->keys[k]); q += 2; }
    }
}

/* The difficulty a demo was recorded at: replaying at another setting would
   change enemy speed and desync immediately. */
int demo_difficulty(const Demo *d) {
    /* player 1: 1 byte InputDevice + 6 integers, so DiffLevel starts at 13 */
    return get_i16(d->player_info + 13);
}

/* ---- recording --------------------------------------------------------- */

void demo_start(Demo *d, uint32_t seed, const Config *cfg) {
    memset(d, 0, sizeof(*d));
    d->seed      = seed;
    d->game_mode = 0;   /* one player */
    encode_player_info(d->player_info, cfg);
}

bool demo_append(Demo *d, int delx, int dely, uint8_t but) {
    if (d->num_frames == d->cap) {
        int ncap = d->cap ? d->cap * 2 : DEMO_BLOCK_FRAMES;
        DemoFrame *nf = realloc(d->frames, (size_t)ncap * sizeof(DemoFrame));
        if (!nf) return false;
        d->frames = nf;
        d->cap    = ncap;
    }
    DemoFrame *f = &d->frames[d->num_frames++];
    /* Pascal integers are 16-bit; clamp rather than wrap so a wild value can
       never silently become a small one. */
    if (delx >  32767) delx =  32767;
    if (delx < -32768) delx = -32768;
    if (dely >  32767) dely =  32767;
    if (dely < -32768) dely = -32768;
    f->delx = (int16_t)delx;
    f->dely = (int16_t)dely;
    f->but  = but;
    return true;
}

void demo_free(Demo *d) {
    free(d->frames);
    d->frames = NULL;
    d->num_frames = d->cap = 0;
}

/* ---- file I/O ---------------------------------------------------------- */

bool demo_load(Demo *d, const char *path) {
    memset(d, 0, sizeof(*d));

    FILE *f = fopen(path, "rb");
    if (!f) return false;

    uint8_t hdr[DEMO_HEADER_BYTES];
    if (fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return false; }

    d->seed = (uint32_t)hdr[0] | ((uint32_t)hdr[1] << 8)
            | ((uint32_t)hdr[2] << 16) | ((uint32_t)hdr[3] << 24);
    d->game_mode = hdr[4];
    memcpy(d->player_info, hdr + 5, DEMO_PLAYERINFO_BYTES);

    /* Frames run to end of file. The original wrote them in blocks of
       MaxDemoFrames, but the blocks are contiguous, so a linear read is
       equivalent and also copes with a truncated final block. */
    for (;;) {
        uint8_t buf[DEMO_FRAME_BYTES];
        size_t got = fread(buf, 1, sizeof(buf), f);
        if (got != sizeof(buf)) break;
        if (!demo_append(d, get_i16(buf), get_i16(buf + 2), buf[4])) {
            fclose(f);
            demo_free(d);
            return false;
        }
    }
    fclose(f);
    return d->num_frames > 0;
}

bool demo_save(const Demo *d, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    uint8_t hdr[DEMO_HEADER_BYTES];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = (uint8_t)( d->seed        & 0xFF);
    hdr[1] = (uint8_t)((d->seed >>  8) & 0xFF);
    hdr[2] = (uint8_t)((d->seed >> 16) & 0xFF);
    hdr[3] = (uint8_t)((d->seed >> 24) & 0xFF);
    hdr[4] = d->game_mode;
    memcpy(hdr + 5, d->player_info, DEMO_PLAYERINFO_BYTES);
    if (fwrite(hdr, 1, sizeof(hdr), f) != sizeof(hdr)) { fclose(f); return false; }

    for (int i = 0; i < d->num_frames; i++) {
        uint8_t buf[DEMO_FRAME_BYTES];
        put_u16(buf,     d->frames[i].delx);
        put_u16(buf + 2, d->frames[i].dely);
        buf[4] = d->frames[i].but;
        if (fwrite(buf, 1, sizeof(buf), f) != sizeof(buf)) { fclose(f); return false; }
    }

    bool ok = (ferror(f) == 0);
    if (fclose(f) != 0) ok = false;
    return ok;
}
