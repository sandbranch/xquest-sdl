#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "assets.h"
#include "entities.h"

/* World dimensions from xqvars.pas */
#define WORLD_W          392
#define WORLD_H          320
#define VIEWPORT_W       320
#define VIEWPORT_H       217   /* SplitScreenLine */
#define HUD_H             23   /* 240 - 217 */

#define MAX_VISIBLE_X     72   /* PageWidth - PhysicalPageWidth */
#define MAX_VISIBLE_Y    103   /* PageHeight - SplitScreenLine */
#define SCREEN_H_BORDER  140   /* PhysicalPageWidth/2 - 20 */
#define SCREEN_V_BORDER  108   /* SplitScreenLine/2 */

/* Ship bounds and start (xqvars.pas) */
#define SHIP_MIN_X    10
#define SHIP_MAX_X   381
#define SHIP_MIN_Y    10
#define SHIP_MAX_Y   309
#define SHIP_START_X 196   /* PageWidth  div 2 */
#define SHIP_START_Y 160   /* PageHeight div 2 */

/* Physics */
#define MAX_SHIP_SPEED  640
#define BASE_GAME_SPEED  64
#define H_INPUT_SPEED    64
#define V_INPUT_SPEED    64
#define KEYBOARD_STEP    10

/* Object placement bounds (MineXMin/Max, MineYMin/Max in xqvars.pas) */
#define MINE_X_MIN    11
#define MINE_X_MAX   371   /* PageWidth  - 21 */
#define MINE_Y_MIN    14
#define MINE_Y_MAX   299   /* PageHeight - 21 */
#define OBJ_W         11   /* all collectibles are 11x11 (crystal/mine/smart_bomb) */
#define OBJ_H         11

/* Game limits */
#define MAX_OBJECTS   65   /* MaxObjects from xqvars.pas */
#define MAX_LEVELS    50
#define START_LIVES    3   /* StartLives */
#define START_BOMBS    3   /* StartBombs */

/* Powerup indices (match Pascal enum Shield..Bounce order) */
#define PU_SHIELD  0
#define PU_AIMED   1
#define PU_RAPID   2
#define PU_MULTI   3
#define PU_ASS     4
#define PU_HEAVY   5
#define PU_BOUNCE  6
#define PU_COUNT   7

/* Timer durations (ticks at 67 fps, from xqvars.pas TimeMin/TimeRan) */
#define PU_TIMEMIN_SHIELD  670   /* 10 * 67 */
#define PU_TIMRAN_SHIELD  1005   /* 15 * 67 */
#define PU_TIMEMIN_AIMED  2010   /* 30 * 67 */
#define PU_TIMRAN_AIMED   4020   /* 60 * 67 */
#define PU_TIMEMIN_LONG   4020   /* 60 * 67 — Rapid/Multi/Ass/Heavy */
#define PU_TIMRAN_LONG    6030   /* 90 * 67 */
#define PU_TIMEMIN_BOUNCE 2010   /* 30 * 67 */
#define PU_TIMRAN_BOUNCE  4020   /* 60 * 67 */

typedef struct {
    int sx, sy;      /* fixed-point world position (pixel * 64) */
    int x,  y;       /* pixel world position */
    int xbr, ybr;    /* bottom-right corner */
    int delx, dely;  /* velocity (fixed-point units) */
    int dir;         /* rotation frame 0-23 */
    int w, h;        /* sprite size */
} Ship;

typedef enum {
    OBJ_CRYSTAL    = 0,
    OBJ_MINE       = 1,
    OBJ_SMART      = 2,
    OBJ_ENEMY_MINE = 3   /* dropped by Miner enemies during play */
} ObjType;

typedef struct {
    int     x, y, xbr, ybr;
    ObjType type;
    bool    active;
} Object;

typedef struct {
    int    numcryst;
    int    nummine;
    int    maxsmart;
    float  smartprob;
    int    newman;
    int    maxenemies;
    float  erelease;
    int    gate_width;
    int    gate_move;
    float  gate_cdir_prob;
    int    time;
    bool   bonus_level;
} LevelRecord;

extern const LevelRecord g_levels[MAX_LEVELS];

typedef struct {
    Ship     ship;
    int      cam_x, cam_y;

    /* Level state */
    int      level;           /* 1-based */
    long     score;
    int      lives;
    int      bombs;           /* smart-bomb inventory (NumSmartBombs) */
    int      num_crystals;    /* remaining crystals to collect */
    bool     gate_open;
    int      gate_left_x;     /* world X of left gate sprite */
    int      gate_right_x;    /* world X of right gate sprite */

    /* Map objects */
    Object   objects[MAX_OBJECTS];
    int      num_objects;

    /* Player missiles */
    Missile      missiles[MAX_MISSILES];
    int          num_missiles;
    int          frame_count;   /* incremented each tick, used for RapidFire timing */

    /* Enemies */
    Enemy        enemies[MAX_ENEMIES];
    int          num_enemies;
    EnemyMissile emissiles[MAX_ENEMY_MISSILES];
    int          num_emissiles;

    /* Gate animation (countdown 80..1 = animating, 0 = idle) */
    int          enemy_entering_left,  enemy_left_type;
    int          enemy_entering_right, enemy_right_type;

    /* Powerup timers — countdown in ticks, 0 = inactive */
    int      powerup_timer[PU_COUNT];

    /* Difficulty / speed */
    int      diff_level;      /* 0-4, default 2 = Average */
    int      gamespeed;       /* round(64 * speedfactor) */

    /* RNG state (xorshift32) */
    uint32_t rng;

    bool ship_destroyed;
    int  ship_explode_timer;   /* counts down SHIP_EXPLODE_TICKS..0 while dying */

    int  gate_inner_x;         /* left edge of gate opening in world coords */
    bool level_complete;       /* set when ship flies through open gate */

    /* Moving gate (levels 33+, from LevelRecord.gate_move / gate_cdir_prob) */
    int  gate_move_accum;      /* 8:8 fixed-point accumulator (Pascal GateMoveCount) */
    int  gate_move_dir;        /* +1 = moving right, -1 = moving left */

    /* Smart-bomb flash: counts 11→0 (Pascal SmartBombed), decrement every other tick */
    int  smart_bomb_flash;
} GameState;

#define SHIP_EXPLODE_TICKS 40  /* freeze duration after ship death */

/* Initialise game state. sprite_w/h are the ship sprite dimensions.
   rng_seed: pass SDL_GetTicks() or similar for variety. */
void game_init(GameState *g, int sprite_w, int sprite_h, uint32_t rng_seed);

/* Set up level g->level: gate positions, place objects. Call after game_init.
   gate_left_w / gate_right_w are the gate sprite widths from Assets. */
void level_init(GameState *g, int gate_left_w, int gate_right_w);

/* Advance one tick. inp_dx/dy from keyboard/mouse. brake halves velocity. */
void game_tick(GameState *g, int inp_dx, int inp_dy, bool brake);

/* Run one tick of enemy spawning and movement. */
void entities_tick(GameState *g, const Assets *a);

/* Reset per-level entity state (called from level_init). */
void entities_level_init(GameState *g);

/* Fire one shot respecting all active powerups (AimedFire, MultiFire, AssFire).
   Matches Pascal Shoot(ship.delx*2, ship.dely*2). */
void shoot(GameState *g, const Assets *a);

/* Decrement all active powerup timers by one tick. */
void powerups_tick(GameState *g);

/* Advance all player missiles one tick (move, expire OOB). */
void missiles_tick(GameState *g, const Assets *a);

/* Check and collect any objects the ship overlaps.
   Crystals: +200 score, decrement num_crystals, open gate at 0.
   Smart bombs: increment bombs inventory. */
void level_check_pickups(GameState *g);

/* Damage enemy at idx by one hit. If HP reaches 0: award score, spawn tribbles /
   explosion as appropriate, mark inactive. dx/dy = impacting velocity (for
   future shootback direction). */
void enemy_hit(GameState *g, const Assets *a, int idx, int dx, int dy);

/* Trigger ship destruction: spawn explosion cluster at ship position and
   start the ship_explode_timer countdown. */
void ship_destroy(GameState *g, const Assets *a);

/* Advance all enemy missiles one tick: move, handle wall rebound/OOB, compact. */
void emissiles_tick(GameState *g, const Assets *a);

/* Pixel-accurate collision detection: missiles vs enemies, ship vs enemies,
   enemy missiles vs ship. Calls enemy_hit on lethal collisions; sets
   g->ship_destroyed if ship is hit. */
void check_collisions(GameState *g, const Assets *a);

/* Fire a smart bomb: kill all enemies, clear enemy missiles, decrement
   bombs inventory, start screen-flash timer. No-op if bombs == 0. */
void fire_smart_bomb(GameState *g, const Assets *a);
