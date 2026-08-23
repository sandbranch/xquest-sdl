#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifndef ENEMY_KINDS
#define ENEMY_KINDS     19    /* indices 0..18: 0=supercrystal, 1=explosion, 2..18=enemies */
#endif

#define MAX_ENEMIES          40
#define MAX_ENEMY_MISSILES   70
#define ENEMY_ENTER_TICKS    80   /* gate countdown before enemy materialises */
#define ENEMY_X_MIN          10
#define ENEMY_X_MAX         382   /* PageWidth - 10 */
#define ENEMY_Y_MIN          10
#define ENEMY_Y_MAX         310   /* PageHeight - 10 */
#define ENEMY_START_Y       155   /* PageHeight/2 - 5 */
#define MAX_MISSILES        50
#define MISSILE_LIFE       300   /* ticks before auto-expiry */
#define DIFF_LEVELS           5

/* Per-type static parameters — ported from enemykindtype in xqvars.pas.
   Width/height come from the sprite at runtime; not stored here. */
typedef struct {
    int    speed, speed2;
    int    curve, curve2;
    int    hits;
    int    firetype;
    int    score;
    bool   fires, follows, curves, explodes, laysmines;
    bool   shootback, zoom, maxspeed, rebounds, tribbles, repulses;
    float  fireprob, changedir, changecurve, follow;
    int    numframes, framespeed;
    int    death_sound;   /* SND_* constant; 0 = silent; explosn → +random(3) variant */
} EnemyKind;

/* Per-instance enemy state — ported from enemytype in xqvars.pas. */
typedef struct {
    int   sx, sy;           /* fixed-point position (world_x * 64) */
    int   x, y, xbr, ybr;  /* pixel bounding box */
    int   delx, dely;       /* velocity (fixed-point units) */
    int   curvecos, curvesin;
    int   type_idx;
    int   hit;
    int   frame;            /* animation counter; sprite index = frame >> 8 */
    int   supertime;        /* ticks remaining for explosions/supercrystals */
    bool  active;
} Enemy;

/* Player missile — one entry per active shot. */
typedef struct {
    int   sx, sy;           /* fixed-point position */
    int   x, y, xbr, ybr;
    int   delx, dely;
    int   time;             /* ticks alive */
    bool  active;
} Missile;

/* Per-instance enemy missile state. */
typedef struct {
    int   sx, sy;
    int   x, y, xbr, ybr;
    int   delx, dely;
    int   firetype;
    bool  active;
} EnemyMissile;

typedef struct {
    float speedfactor;
    float enemyfrequency;
    bool  rebound;          /* on Wimp/Timid: ship bounces off walls */
} DiffLevel;

extern const EnemyKind  g_enemy_kinds[ENEMY_KINDS];
extern const uint8_t    g_probs[50][ENEMY_KINDS];   /* spawn weight table from xqvars.pas */
extern const DiffLevel  g_diff_levels[DIFF_LEVELS];
