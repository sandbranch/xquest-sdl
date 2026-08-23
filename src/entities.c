#include "game.h"
#include "entities.h"
#include "assets.h"
#include "audio.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Enemy kind table — ported verbatim from xqenter.pas.
   Index 0 = supercrystal, 1 = explosion, 2..18 = regular enemies. */
/* death_sound values (SND_* from audio.h):
   12=SND_EXPLOSN (default, plays +random 0-2 for variants 12-14)
   19=SND_SXTSMASH, 15=SND_RETALIATE, 0=silent */
const EnemyKind g_enemy_kinds[ENEMY_KINDS] = {
/* 0 supercrystal */
 {301,150, 0,   0, 1,0,    0, false,false,false,false,false,false,false, true,false,false,false,0.0f,   0.002f,0.0f,  0.0f,  5,70, 19},
/* 1 explosion */
 {  0,  0, 0,   0, 1,0,    0, false,false,false,false,false,false,false,false,false,false,false,0.0f,   0.0f,  0.0f,  0.0f,  5,64,  0},
/* 2 grunger */
 {121, 60, 0,   0, 1,0,  200, false,false,false,false,false,false,false,false,false,false,false,0.0f,   0.006f,0.0f,  0.0f,  3,40, 12},
/* 3 zippo */
 {281,140,6000,3000,1,0, 300, false,false, true,false,false,false,false,false,false,false,false,0.0f,   0.003f,0.02f, 0.0f,  3,56, 12},
/* 4 zinger */
 {101, 50, 0,   0, 1,1,  300,  true,false,false,false,false,false,false,false,false,false,false,0.01f,  0.006f,0.0f,  0.0f,  3,60, 12},
/* 5 vince */
 {201,100, 0,   0, 1,0,  500, false,false,false,false,false,false,false,false, true,false,false,0.0f,   0.003f,0.0f,  0.0f,  3,56, 12},
/* 6 hibernator (hibernating) */
 {  0,  0, 0,   0,300,0, 500, false,false,false,false,false,false,false,false,false,false,false,0.0f,   0.0f,  0.0f,  0.0f,  0,32767,12},
/* 7 miner */
 {121, 60,4000,2000,1,0, 600, false,false, true,false, true,false,false,false,false,false,false,0.008f, 0.006f,0.1f,  0.0f,  3,32, 12},
/* 8 meeby */
 { 81, 40, 0,   0, 5,0, 2000, false, true,false,false,false,false,false,false,false,false,false,0.0f,   0.006f,0.0f,  0.01f, 5,28, 12},
/* 9 retaliator */
 {121, 60, 0,   0, 1,3, 1000, false,false,false,false,false, true,false,false,false,false,false,0.0f,   0.006f,0.0f,  0.0f,  3,64,  0},
/*10 terrier */
 {121, 60, 0,   0, 1,0, 1000, false,false,false,false,false,false, true,false,false,false,false,0.0f,   0.02f, 0.0f,  0.0f,  3,120,12},
/*11 doinger */
 {121, 60, 0,   0, 1,4, 1000,  true,false,false,false,false,false,false,false,false,false,false,0.005f, 0.003f,0.0f,  0.0f,  3,128,12},
/*12 snipe */
 {131, 65, 0,   0, 1,5, 1250,  true,false,false,false,false,false,false,false,false,false,false,0.004f, 0.004f,0.0f,  0.0f,  3,26, 12},
/*13 tribbler */
 {100, 50, 0,   0, 1,0, 1500, false,false,false,false,false,false,false,false,false, true,false,0.0f,   0.01f, 0.0f,  0.0f,  3,32, 12},
/*14 tribble */
 {220,110,1000,500, 1,0,  500, false,false, true,false,false,false,false,false,false,false,false,0.0f,   0.005f,0.1f,  0.0f,  3,48, 12},
/*15 buckshot */
 {101, 50, 0,   0, 1,2, 1500,  true,false,false,false,false,false,false,false,false,false,false,0.03f,  0.006f,0.0f,  0.0f,  3,36, 12},
/*16 cluster */
 { 81, 40, 0,   0, 1,6, 5000, false,false,false, true,false,false,false,false,false,false,false,0.0f,   0.02f, 0.0f,  0.0f,  3,32, 15},
/*17 sticktight */
 {101, 50, 0,   0, 1,0, 2000, false, true,false,false,false,false,false,false,false,false,false,0.0f,   0.0f,  0.0f,  1.0f,  3,40, 12},
/*18 repulsor */
 {141, 70, 0,   0, 1,0, 7500, false, true,false,false,false,false,false,false,false,false, true,0.0f,   0.01f, 0.0f,  0.01f, 5,60, 12},
};

/* Spawn probability table — verbatim from xqvars.pas probs[1..50][0..18].
   g_probs[level-1][kind_index]. Value is the threshold for random(100). */
const uint8_t g_probs[50][ENEMY_KINDS] = {
/*1 */ { 5,0, 60, 0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*2 */ { 5,0,100, 0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*3 */ { 5,0,  0,100,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*4 */ { 5,0, 15, 85,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*5 */ { 7,0,  0,  0,100,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*6 */ { 7,0, 15, 15, 70,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*7 */ { 7,0,  0,  0,  0,100,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*8 */ { 7,0, 15, 15, 15, 55,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*9 */ { 7,0,  0,  0,  0,  0,0,100,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*10*/ { 7,0, 15, 15, 15, 15,0, 50,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*11*/ { 7,0,  0,  0,  0,  0,0,  0,100,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*12*/ { 7,0, 10, 10, 10, 10,0, 10, 60,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*13*/ { 7,0,  0,  0,  0,  0,0,  0,  0,100,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*14*/ { 7,0, 10, 10, 10, 10,0, 10,  3, 60,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*15*/ { 7,0,  0,  0,  0,  0,0,  0,  0,  0,100,  0,  0,  0, 0,  0,  0,  0,  0},
/*16*/ { 7,0, 10, 10, 10, 10,0, 10,  3,  3, 60,  0,  0,  0, 0,  0,  0,  0,  0},
/*17*/ { 7,0,  0,  0,  0,  0,0,  0,  0,  0,  0,100,  0,  0, 0,  0,  0,  0,  0},
/*18*/ { 7,0, 10, 10, 10, 10,0, 10, 10,  3,  3, 60,  0,  0, 0,  0,  0,  0,  0},
/*19*/ { 7,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,100,  0, 0,  0,  0,  0,  0},
/*20*/ {10,0, 10, 10, 10, 10,0, 10, 10,  5,  3,  3, 60,  0, 0,  0,  0,  0,  0},
/*21*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,100, 0,  0,  0,  0,  0},
/*22*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10,  5,  3,  3, 60, 0,  0,  0,  0,  0},
/*23*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,100,  0,  0,  0},
/*24*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10,  5,  5,  3,  3, 0, 60,  0,  0,  0},
/*25*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,100,  0,  0},
/*26*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10,  5,  5,  3, 0,  3, 60,  0,  0},
/*27*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,100,  0},
/*28*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10,  5,  5,  5, 0,  3,  3, 60,  0},
/*29*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10,  5,  5, 0,  3,  3, 50,  0},
/*30*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0,  5,  5, 40,  0},
/*31*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 30,  0},
/*32*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,100},
/*33*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10,  5,  5,  5, 0,  3,  3,  3, 60},
/*34*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0,  3,  3,  3, 40},
/*35*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10,  3, 20},
/*36*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*37*/ {10,0,  0,100,  0,100,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0,  0,  0},
/*38*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*39*/ {10,0,  0,  0,  0,  0,0, 50,  0,  0,  0,  0,  0,  0, 0,  0,  0, 50,  0},
/*40*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*41*/ {10,0,  0,  0,  0,  0,0,  0, 50,  0,  0,  0,  0, 50, 0,  0,  0,  0,  0},
/*42*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*43*/ {10,0,  0,  0,  0,  0,0,  0,  0, 50,  0, 50,  0,  0, 0,  0,  0,  0,  0},
/*44*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*45*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0, 50,  0, 50,  0, 0,  0,  0,  0,  0},
/*46*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*47*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0, 50, 50,  0,  0},
/*48*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
/*49*/ {10,0,  0,  0,  0,  0,0,  0,  0,  0,  0,  0,  0,  0, 0,  0,  0, 50, 50},
/*50*/ {10,0, 10, 10, 10, 10,0, 10, 10, 10, 10, 10, 10, 10, 0, 10, 10, 10, 10},
};

const DiffLevel g_diff_levels[DIFF_LEVELS] = {
    {0.7f, 0.7f,  true},   /* 0 Wimp    */
    {1.0f, 1.0f,  true},   /* 1 Timid   */
    {1.0f, 1.0f, false},   /* 2 Average */
    {1.5f, 1.2f, false},   /* 3 Tricky  */
    {2.0f, 1.5f, false},   /* 4 Inhuman */
};

/* xorshift32 — same generator used in starfield.c and game.c */
static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return (*s = x);
}

static float rng_float(uint32_t *s) {
    return (float)(rng_next(s) >> 8) / (float)(1 << 24);
}

static int rng_range(uint32_t *s, int n) {
    return (int)(rng_next(s) % (unsigned)n);
}

void entities_level_init(GameState *g) {
    g->num_missiles         = 0;
    g->num_enemies          = 0;
    g->num_emissiles        = 0;
    g->enemy_entering_left  = 0;
    g->enemy_entering_right = 0;
    g->enemy_left_type      = 0;
    g->enemy_right_type     = 0;
    memset(g->missiles,  0, sizeof(g->missiles));
    memset(g->enemies,   0, sizeof(g->enemies));
    memset(g->emissiles, 0, sizeof(g->emissiles));
}

static void add_enemy(GameState *g, const Assets *a, int xc, int yc, int kind) {
    if (g->num_enemies >= MAX_ENEMIES) return;
    const EnemyKind *k = &g_enemy_kinds[kind];
    int w = a->enemy[kind][0].w;
    int h = a->enemy[kind][0].h;

    Enemy *e = &g->enemies[g->num_enemies++];
    memset(e, 0, sizeof(*e));

    e->type_idx = kind;
    e->hit      = k->hits;
    e->active   = true;
    e->x  = xc; e->xbr = xc + w - 1;
    e->y  = yc; e->ybr = yc + h - 1;
    e->sx = xc * 64;
    e->sy = yc * 64;

    /* Initial velocity */
    int gs = g->gamespeed;
    if (!k->maxspeed) {
        if (k->speed > 0) {
            e->delx = (rng_range(&g->rng, k->speed) - k->speed2) * gs / 64;
            e->dely = (rng_range(&g->rng, k->speed) - k->speed2) * gs / 64;
        }
    } else {
        double t = cos(rng_float(&g->rng) * 2.0 * M_PI - M_PI);
        e->delx = (int)round(k->speed * t) * gs / 64;
        e->dely = (int)round(k->speed * sqrt(1.0 - t * t)) * gs / 64;
    }

    /* Initial curve rotation parameters */
    if (k->curve > 0) {
        e->curvesin = -k->curve2 + rng_range(&g->rng, k->curve);
        double cs = sqrt(1.0 - ((double)e->curvesin / 32767.0) * ((double)e->curvesin / 32767.0));
        e->curvecos = (int)round(cs * 32767.0);
    } else {
        e->curvesin = 0;
        e->curvecos = 32767;
    }

    /* Supertime: explosions live one animation cycle; supercrystals time out */
    if (kind == 1)
        e->supertime = k->numframes * (256 / k->framespeed);
    else if (kind == 0)
        e->supertime = 335 + rng_range(&g->rng, 335);   /* SuperTimeMin + random(SuperTimeRan) */
    else
        e->supertime = 0;
}

static void fire_missile_raw(GameState *g, const Assets *a, int dx, int dy) {
    if (g->num_missiles >= MAX_MISSILES) return;
    int mw = a->player_missile.w;
    int mh = a->player_missile.h;
    Missile *m = &g->missiles[g->num_missiles++];
    m->sx   = g->ship.sx + (g->ship.w << 5) - (mw << 4);
    m->sy   = g->ship.sy + (g->ship.h << 5) - (mh << 4);
    m->x    = m->sx / 64;
    m->y    = m->sy / 64;
    m->xbr  = m->x + mw - 1;
    m->ybr  = m->y + mh - 1;
    m->delx = dx;
    m->dely = dy;
    m->time = 0;
    m->active = true;
}

void shoot(GameState *g, const Assets *a) {
    int dx = g->ship.delx * 2;
    int dy = g->ship.dely * 2;

    /* AimedFire: override direction toward nearest enemy with lead calculation */
    if (g->powerup_timer[PU_AIMED] > 0 && g->num_enemies > 0) {
        const int AIMED_SPEED = 256;  /* fixed-point, 4 px/tick */
        long mindist2 = 0x7FFFFFFF;
        int nearest = -1;
        for (int i = 0; i < g->num_enemies; i++) {
            if (!g->enemies[i].active || g->enemies[i].type_idx == 1) continue;
            long ddx = g->ship.x - g->enemies[i].x;
            long ddy = g->ship.y - g->enemies[i].y;
            long d2 = ddx * ddx + ddy * ddy;
            if (d2 < mindist2) { mindist2 = d2; nearest = i; }
        }
        if (nearest >= 0 && mindist2 > 0) {
            long md = (long)sqrt((double)mindist2);
            if (md < 1) md = 1;
            int fd = (int)(md / (AIMED_SPEED / 64));
            int ex = g->enemies[nearest].x + (fd * g->enemies[nearest].delx) / 64;
            int ey = g->enemies[nearest].y + (fd * g->enemies[nearest].dely) / 64;
            dx = (int)((long)AIMED_SPEED * (ex - g->ship.x) / md);
            dy = (int)((long)AIMED_SPEED * (ey - g->ship.y) / md);
        }
    }

    audio_play(g->powerup_timer[PU_MULTI] > 0 ? SND_FIRE4 : SND_FIRE);
    fire_missile_raw(g, a, dx, dy);

    if (g->powerup_timer[PU_ASS] > 0)
        fire_missile_raw(g, a, -dx, -dy);

    if (g->powerup_timer[PU_MULTI] > 0) {
        double r     = sqrt((double)dx * dx + (double)dy * dy);
        double theta = atan2((double)dy, (double)dx);
        const double delta = 3.14159265358979323846 / 180.0 * 10.0;
        fire_missile_raw(g, a, (int)(r * cos(theta + delta)), (int)(r * sin(theta + delta)));
        fire_missile_raw(g, a, (int)(r * cos(theta - delta)), (int)(r * sin(theta - delta)));
        if (g->powerup_timer[PU_ASS] > 0) {
            fire_missile_raw(g, a, -(int)(r * cos(theta + delta)), -(int)(r * sin(theta + delta)));
            fire_missile_raw(g, a, -(int)(r * cos(theta - delta)), -(int)(r * sin(theta - delta)));
        }
    }
}

void missiles_tick(GameState *g, const Assets *a) {
    int mw = a->player_missile.w;
    int mh = a->player_missile.h;

    for (int i = 0; i < g->num_missiles; i++) {
        Missile *m = &g->missiles[i];
        if (!m->active) continue;

        m->sx  += m->delx;
        m->sy  += m->dely;
        m->x    = m->sx / 64;
        m->y    = m->sy / 64;
        m->xbr  = m->x + mw - 1;
        m->ybr  = m->y + mh - 1;
        m->time++;

        bool bounce = g->powerup_timer[PU_BOUNCE] > 0;
        if (bounce) {
            bool hit = false;
            if (m->x < 10)          { m->delx =  abs(m->delx); m->sx = 10 * 64; m->x = 10; hit = true; }
            if (m->xbr > WORLD_W-10){ m->delx = -abs(m->delx); m->x = WORLD_W-10-mw; m->sx = m->x*64; m->xbr = m->x+mw-1; hit = true; }
            if (m->y < 10)          { m->dely =  abs(m->dely); m->sy = 10 * 64; m->y = 10; hit = true; }
            if (m->ybr > WORLD_H-10){ m->dely = -abs(m->dely); m->y = WORLD_H-10-mh; m->sy = m->y*64; m->ybr = m->y+mh-1; hit = true; }
            if (hit) audio_play(SND_BOING);
        } else if (m->x < 10 || m->xbr > WORLD_W - 10 ||
                   m->y < 10 || m->ybr > WORLD_H - 10) {
            m->active = false;
        }
        if (m->time > MISSILE_LIFE) m->active = false;
    }

    /* Compact */
    for (int i = g->num_missiles - 1; i >= 0; i--) {
        if (!g->missiles[i].active)
            g->missiles[i] = g->missiles[--g->num_missiles];
    }
}

/* Per-firetype parameters — index 0 unused (firetype 1..6). */
static const int  em_speed[7]   = {0, 120, 150, 200, 150, 150, 170};
static const bool em_direct[7]  = {false, false, false, true, false, true, false};
static const bool em_rebound[7] = {false, false, false, false, true, false, false};

static void fire_emissile(GameState *g, const Assets *a, int ei) {
    if (g->num_emissiles >= MAX_ENEMY_MISSILES) return;
    Enemy *e = &g->enemies[ei];
    int ftype = g_enemy_kinds[e->type_idx].firetype;
    if (ftype < 1 || ftype > 6) return;

    int ew = a->enemy[e->type_idx][0].w;
    int eh = a->enemy[e->type_idx][0].h;
    int mw = a->enemy_missile[ftype - 1].w;
    int mh = a->enemy_missile[ftype - 1].h;
    int gs = g->gamespeed;
    int spd = em_speed[ftype];

    EnemyMissile *em = &g->emissiles[g->num_emissiles++];
    memset(em, 0, sizeof(*em));
    em->firetype = ftype;
    em->active   = true;
    /* Centre missile on enemy centre */
    em->sx  = e->sx + (ew << 5) - (mw << 4);
    em->sy  = e->sy + (eh << 5) - (mh << 4);
    em->x   = em->sx / 64;
    em->y   = em->sy / 64;
    em->xbr = em->x + mw - 1;
    em->ybr = em->y + mh - 1;

    if (em_direct[ftype]) {
        /* Aimed at ship centre */
        int dx = g->ship.x + g->ship.w / 2 - em->x;
        int dy = g->ship.y + g->ship.h / 2 - em->y;
        double mag = sqrt((double)dx * dx + (double)dy * dy);
        if (mag < 1.0) mag = 1.0;
        em->delx = (int)((double)dx / mag * spd) * gs / 64;
        em->dely = (int)((double)dy / mag * spd) * gs / 64;
    } else {
        /* Random spread — matches Pascal: random(mspeed) - mspeed/2 */
        em->delx = (rng_range(&g->rng, spd) - spd / 2) * gs / 64;
        em->dely = (rng_range(&g->rng, spd) - spd / 2) * gs / 64;
    }
}

/* Drop an enemy mine at the Miner's centre (Pascal AddEnemyMine). */
static void add_enemy_mine(GameState *g, const Assets *a, int idx) {
    if (g->num_objects >= MAX_OBJECTS) return;
    Enemy *e = &g->enemies[idx];
    int ew = a->enemy[e->type_idx][0].w;
    int mw = a->enemy_mine.w;
    int mh = a->enemy_mine.h;
    Object *o = &g->objects[g->num_objects++];
    o->x   = e->x + ew / 2;
    o->y   = e->y + ew / 2;
    o->xbr = o->x + mw - 1;
    o->ybr = o->y + mh - 1;
    o->type   = OBJ_ENEMY_MINE;
    o->active = true;
}

void emissiles_tick(GameState *g, const Assets *a) {
    for (int i = 0; i < g->num_emissiles; i++) {
        EnemyMissile *em = &g->emissiles[i];
        if (!em->active) continue;
        int mw = a->enemy_missile[em->firetype - 1].w;
        int mh = a->enemy_missile[em->firetype - 1].h;
        bool rb = em_rebound[em->firetype];

        em->sx += em->delx;
        em->sy += em->dely;
        em->x   = em->sx / 64;
        em->y   = em->sy / 64;
        em->xbr = em->x + mw - 1;
        em->ybr = em->y + mh - 1;

        if (em->x < ENEMY_X_MIN) {
            if (rb) { em->delx = abs(em->delx); em->x = ENEMY_X_MIN; em->sx = em->x * 64; }
            else { em->active = false; continue; }
        } else if (em->xbr > ENEMY_X_MAX) {
            if (rb) { em->delx = -abs(em->delx); em->x = ENEMY_X_MAX - mw; em->sx = em->x * 64; }
            else { em->active = false; continue; }
        }
        if (em->y < ENEMY_Y_MIN) {
            if (rb) { em->dely = abs(em->dely); em->y = ENEMY_Y_MIN; em->sy = em->y * 64; }
            else { em->active = false; continue; }
        } else if (em->ybr > ENEMY_Y_MAX) {
            if (rb) { em->dely = -abs(em->dely); em->y = ENEMY_Y_MAX - mh; em->sy = em->y * 64; }
            else { em->active = false; continue; }
        }
    }
    /* Compact */
    for (int i = g->num_emissiles - 1; i >= 0; i--) {
        if (!g->emissiles[i].active)
            g->emissiles[i] = g->emissiles[--g->num_emissiles];
    }
}

void entities_tick(GameState *g, const Assets *a) {
    g->frame_count++;
    const LevelRecord *rec  = &g_levels[g->level - 1];
    const DiffLevel   *diff = &g_diff_levels[g->diff_level];
    int gs = g->gamespeed;

    /* --- Spawn check: each kind independently rolls for entry --- */
    for (int i = 0; i < ENEMY_KINDS; i++) {
        if (g->num_enemies >= rec->maxenemies) break;
        if (rng_float(&g->rng) >= rec->erelease * diff->enemyfrequency) continue;
        if ((int)rng_range(&g->rng, 100) >= (int)g_probs[g->level - 1][i]) continue;

        if (rng_range(&g->rng, 2) == 1 && g->enemy_entering_left == 0) {
            g->enemy_entering_left = ENEMY_ENTER_TICKS;
            g->enemy_left_type     = i;
        } else if (g->enemy_entering_right == 0) {
            g->enemy_entering_right = ENEMY_ENTER_TICKS;
            g->enemy_right_type     = i;
        }
    }

    /* --- Left gate countdown --- */
    if (g->enemy_entering_left > 0) {
        g->enemy_entering_left--;
        if (g->enemy_entering_left == 0) {
            audio_play(SND_ENEMYENT);
            add_enemy(g, a, 15, ENEMY_START_Y, g->enemy_left_type);
        }
    }

    /* --- Right gate countdown --- */
    if (g->enemy_entering_right > 0) {
        g->enemy_entering_right--;
        if (g->enemy_entering_right == 0) {
            int kind = g->enemy_right_type;
            int w    = a->enemy[kind][0].w;
            audio_play(SND_ENEMYENT);
            add_enemy(g, a, WORLD_W - w - 16, ENEMY_START_Y, kind);
        }
    }

    /* --- Move each active enemy --- */
    for (int i = 0; i < g->num_enemies; i++) {
        Enemy *e = &g->enemies[i];
        if (!e->active) continue;
        const EnemyKind *k = &g_enemy_kinds[e->type_idx];
        int w = a->enemy[e->type_idx][0].w;
        int h = a->enemy[e->type_idx][0].h;

        /* Animation */
        e->frame += k->framespeed;
        if (e->frame >= (k->numframes + 1) << 8) e->frame = 0;

        /* Velocity update */
        if (k->follows && rng_float(&g->rng) < k->follow) {
            int dx = g->ship.x - e->x;
            int dy = g->ship.y - e->y;
            int mag = abs(dx) + abs(dy);
            if (mag > 0) {
                e->delx = (int)((long)dx * k->speed / mag) * gs / 64;
                e->dely = (int)((long)dy * k->speed / mag) * gs / 64;
            }
        } else if (rng_float(&g->rng) < k->changedir) {
            if (k->zoom && (abs(g->ship.delx) + abs(g->ship.dely)) < 60) {
                /* Terrier zoom: charge at 1.5x speed */
                int dx = g->ship.x - e->x;
                int dy = g->ship.y - e->y;
                int mag = abs(dx) + abs(dy);
                if (mag > 0) {
                    e->delx = (int)((long)dx * k->speed / mag) * gs / 48;
                    e->dely = (int)((long)dy * k->speed / mag) * gs / 48;
                }
            } else if (!k->maxspeed) {
                e->delx = (rng_range(&g->rng, k->speed) - k->speed2) * gs / 64;
                e->dely = (rng_range(&g->rng, k->speed) - k->speed2) * gs / 64;
            } else {
                double t = cos(rng_float(&g->rng) * 2.0 * M_PI - M_PI);
                e->delx = (int)round(k->speed * t) * gs / 64;
                e->dely = (int)round(k->speed * sqrt(1.0 - t * t)) * gs / 64;
            }
        }

        /* Curve rotation (rotation matrix with integer trig, 32767 = 1.0) */
        if (k->curves) {
            if (rng_float(&g->rng) < k->changecurve) {
                e->curvesin = -k->curve2 + rng_range(&g->rng, k->curve);
                double cs = sqrt(1.0 - ((double)e->curvesin / 32767.0) * ((double)e->curvesin / 32767.0));
                e->curvecos = (int)round(cs * 32767.0);
            }
            int old_dx = e->delx;
            long lt;
            lt = (long)e->delx * e->curvecos - (long)e->dely * e->curvesin;
            e->delx = (int)(lt > 0 ? (lt + 16384) / 32767 : (lt - 16384) / 32767);
            lt = (long)e->dely * e->curvecos + (long)old_dx * e->curvesin;
            e->dely = (int)(lt > 0 ? (lt + 16384) / 32767 : (lt - 16384) / 32767);
        }

        /* Position update */
        e->sx += e->delx;
        e->sy += e->dely;
        e->x   = e->sx / 64;
        e->y   = e->sy / 64;
        e->xbr = e->x + w - 1;
        e->ybr = e->y + h - 1;

        /* Wall bounce */
        if (e->x < ENEMY_X_MIN)   { e->x = ENEMY_X_MIN;        e->sx = e->x*64; e->delx =  abs(e->delx); }
        if (e->xbr > ENEMY_X_MAX) { e->x = ENEMY_X_MAX - w;    e->sx = e->x*64; e->delx = -abs(e->delx); }
        if (e->y < ENEMY_Y_MIN)   { e->y = ENEMY_Y_MIN;        e->sy = e->y*64; e->dely =  abs(e->dely); }
        if (e->ybr > ENEMY_Y_MAX) { e->y = ENEMY_Y_MAX - h;    e->sy = e->y*64; e->dely = -abs(e->dely); }
        e->xbr = e->x + w - 1;
        e->ybr = e->y + h - 1;

        /* Keep enemies away from the enemy inlet band (mid-height wall edges) */
        int mid = WORLD_H / 2;
        if (e->y < mid + 7 && e->ybr > mid - 7) {
            if (e->x < ENEMY_X_MIN + 7) e->delx = abs(e->delx);
            if (e->xbr > ENEMY_X_MAX - 7) e->delx = -abs(e->delx);
        }

        /* Fire check */
        if (k->fires && rng_float(&g->rng) < k->fireprob)
            fire_emissile(g, a, i);

        /* Mine-laying check (Miner: same fireprob threshold, Pascal line 1981) */
        if (k->laysmines && rng_float(&g->rng) < k->fireprob)
            add_enemy_mine(g, a, i);

        /* Repulsor: push ship away (Pascal EnemyRepel, inverse-square law) */
        if (k->repulses) {
            long dx = g->ship.x - e->x;
            long dy = g->ship.y - e->y;
            if (dx != 0 || dy != 0) {
                long m;
                if (labs(dx) > labs(dy))
                    m = (dx*dx + dy*dy) * (labs(dx) + labs(dy)/2);
                else
                    m = (dx*dx + dy*dy) * (labs(dy) + labs(dx)/2);
                if (m > 0) {
                    if (m < 500000 && (g->frame_count & 31) == 0)
                        audio_play(SND_REPULSE);
                    g->ship.delx += (int)(8192L * dx / m);
                    g->ship.dely += (int)(8192L * dy / m);
                }
            }
        }

        /* Supertime countdown */
        if (e->supertime > 0 && --e->supertime == 0)
            e->active = false;
    }

    /* Compact: swap inactive enemies to the end */
    for (int i = g->num_enemies - 1; i >= 0; i--) {
        if (!g->enemies[i].active)
            g->enemies[i] = g->enemies[--g->num_enemies];
    }
}

void ship_destroy(GameState *g, const Assets *a) {
    audio_play(SND_EXPLOSN);
    /* Spawn 5 overlapping explosion sprites centred on the ship. */
    int ew = a->enemy[1][0].w;
    int eh = a->enemy[1][0].h;
    int cx = g->ship.x + g->ship.w / 2 - ew / 2;
    int cy = g->ship.y + g->ship.h / 2 - eh / 2;
    static const int ox[5] = { 0, -8,  8,  0,  0 };
    static const int oy[5] = { 0,  0,  0, -8,  8 };
    for (int i = 0; i < 5; i++)
        add_enemy(g, a, cx + ox[i], cy + oy[i], 1);
    g->ship_explode_timer = SHIP_EXPLODE_TICKS;
}

/* Drop an enemy mine at the Miner's centre (Pascal AddEnemyMine). */
/* Cluster explosion: fire up to 15 missiles evenly around the full circle. */
static void explode_enemy(GameState *g, const Assets *a, int idx) {
    Enemy *e = &g->enemies[idx];
    int ftype = g_enemy_kinds[e->type_idx].firetype;
    if (ftype < 1 || ftype > 6) return;

    int ew  = a->enemy[e->type_idx][0].w;
    int eh  = a->enemy[e->type_idx][0].h;
    int mw  = a->enemy_missile[ftype - 1].w;
    int mh  = a->enemy_missile[ftype - 1].h;
    int spd = em_speed[ftype] * g->gamespeed / 64;
    int cx  = e->sx + (ew << 5) - (mw << 4);  /* centred on enemy */
    int cy  = e->sy + (eh << 5) - (mh << 4);
    int count = MAX_ENEMY_MISSILES - g->num_emissiles;
    if (count > 15) count = 15;

    for (int i = 0; i < count; i++) {
        if (g->num_emissiles >= MAX_ENEMY_MISSILES) break;
        double theta = 2.0 * M_PI * i / count;
        EnemyMissile *em = &g->emissiles[g->num_emissiles++];
        memset(em, 0, sizeof(*em));
        em->firetype = ftype;
        em->active   = true;
        em->sx  = cx;  em->sy  = cy;
        em->x   = cx / 64;  em->y  = cy / 64;
        em->xbr = em->x + mw - 1;  em->ybr = em->y + mh - 1;
        em->delx = (int)(cos(theta) * spd);
        em->dely = (int)(sin(theta) * spd);
    }
}

void enemy_hit(GameState *g, const Assets *a, int idx, int dx, int dy) {
    Enemy *e = &g->enemies[idx];
    if (g->powerup_timer[PU_HEAVY] > 0) e->hit = 0;  /* HeavyFire: one-shot kill */
    if (--e->hit > 0) return;

    int ex = e->x, ey = e->y;
    int kind = e->type_idx;
    const EnemyKind *k = &g_enemy_kinds[kind];

    /* Rebounds: redirect instead of kill (Pascal EnemyDestroyed rebounds branch).
       HeavyFire bypasses this because hit was forced to 0 above. */
    if (k->rebounds && g->powerup_timer[PU_HEAVY] == 0) {
        e->delx = (3 * e->delx + dx) / 4;
        e->dely = (3 * e->dely + dy) / 4;
        e->hit  = 1;   /* restore HP so it can be hit again */
        return;
    }

    g->score += g_enemy_kinds[e->type_idx].score;

    /* Death sound: explosn plays one of three variants randomly */
    if (k->death_sound == SND_EXPLOSN)
        audio_play(SND_EXPLOSN + rng_range(&g->rng, 3));
    else if (k->death_sound > 0)
        audio_play(k->death_sound);

    /* Retaliator: fire one missile toward player on death */
    if (k->shootback) fire_emissile(g, a, idx);

    /* Cluster: explode outward */
    if (k->explodes) explode_enemy(g, a, idx);

    if (k->tribbles) {
        for (int i = 0; i < 5; i++)
            add_enemy(g, a, ex, ey, kind + 1);
    }

    e->active = false;

    if (kind > 1)
        add_enemy(g, a, ex, ey, 1);
}
