#include "game.h"
#include "audio.h"
#include <math.h>
#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* All 50 levels from xqvars.pas const levels array. */
const LevelRecord g_levels[MAX_LEVELS] = {
    /* numcryst,nummine,maxsmart,smartprob,newman,maxenemies,erelease,gate_width,gate_move,gate_cdir_prob,time,bonus */
    {15, 0,1,0.2f,15000,20,0.005f,27,0,0.0f,20,false}, /*  1 */
    {16, 3,1,0.2f,15000, 4,0.005f,27,0,0.0f,20,false}, /*  2 */
    {17, 4,1,0.2f,15000, 5,0.005f,26,0,0.0f,20,false}, /*  3 */
    {18, 5,1,0.2f,15000, 6,0.005f,26,0,0.0f,25,false}, /*  4 */
    {19, 6,1,0.2f,15000, 7,0.005f,25,0,0.0f,25,false}, /*  5 */
    {20, 6,1,0.2f,15000, 8,0.005f,25,0,0.0f,30,false}, /*  6 */
    {21, 7,1,0.2f,15000, 9,0.005f,24,0,0.0f,30,false}, /*  7 */
    {22, 7,1,0.2f,20000,10,0.006f,24,0,0.0f,35,false}, /*  8 */
    {23, 8,1,0.2f,20000,10,0.006f,23,0,0.0f,35,false}, /*  9 */
    {24, 8,1,0.2f,20000,10,0.006f,23,0,0.0f,40,false}, /* 10 */
    {24, 9,2,0.2f,20000,10,0.006f,22,0,0.0f,40,false}, /* 11 */
    {25, 9,2,0.2f,20000,10,0.007f,22,0,0.0f,45,false}, /* 12 */
    {25,10,2,0.2f,40000,10,0.007f,21,0,0.0f,45,false}, /* 13 */
    {26,10,2,0.2f,40000,10,0.008f,21,0,0.0f,45,false}, /* 14 */
    {26,10,2,0.2f,40000,10,0.008f,20,0,0.0f,50,false}, /* 15 */
    {27,11,2,0.2f,40000,10,0.009f,20,0,0.0f,50,false}, /* 16 */
    {27,11,2,0.2f,40000,10,0.009f,19,0,0.0f,50,false}, /* 17 */
    {28,11,2,0.2f,40000,10,0.010f,19,0,0.0f,55,false}, /* 18 */
    {28,12,2,0.2f,40000,10,0.010f,18,0,0.0f,55,false}, /* 19 */
    {29,12,2,0.3f,40000,10,0.010f,18,0,0.0f,55,false}, /* 20 */
    {29,12,2,0.3f,70000,10,0.010f,17,0,0.0f,60,false}, /* 21 */
    {30,13,2,0.3f,70000,10,0.010f,17,0,0.0f,60,false}, /* 22 */
    {30,13,2,0.3f,70000,10,0.010f,17,0,0.0f,60,false}, /* 23 */
    {31,13,2,0.3f,70000,10,0.010f,17,0,0.0f,60,false}, /* 24 */
    {31,13,2,0.3f,70000,10,0.010f,17,0,0.0f,65,false}, /* 25 */
    {32,14,2,0.3f,70000,10,0.010f,17,0,0.0f,65,false}, /* 26 */
    {32,14,2,0.3f,70000,11,0.010f,17,0,0.0f,65,false}, /* 27 */
    {33,14,2,0.3f,70000,11,0.010f,17,0,0.0f,65,false}, /* 28 */
    {33,14,2,0.3f,70000,12,0.010f,17,0,0.0f,70,false}, /* 29 */
    {34,15,2,0.3f,70000,12,0.010f,17,0,0.0f,70,false}, /* 30 */
    {34,15,2,0.3f,70000,13,0.010f,17,0,0.0f,70,false}, /* 31 */
    {35,15,2,0.3f,70000,13,0.010f,17,0,0.0f,70,false}, /* 32 */
    {35,15,2,0.3f,100000,14,0.010f,17,10,0.000f,75,false}, /* 33 */
    {36,16,2,0.3f,100000,14,0.010f,17,20,0.000f,75,false}, /* 34 */
    {36,16,2,0.3f,100000,15,0.010f,17,30,0.000f,75,false}, /* 35 */
    {37,16,2,0.3f,100000,15,0.010f,17,40,0.000f,75,false}, /* 36 */
    {37,16,2,0.3f,100000,16,0.025f,17,50,0.000f,80,false}, /* 37 */
    {38,17,2,0.3f,100000,16,0.011f,17,60,0.000f,80,false}, /* 38 */
    {38,17,2,0.3f,100000,17,0.012f,17,70,0.000f,80,false}, /* 39 */
    {39,17,2,0.3f,100000,17,0.012f,17,80,0.000f,80,false}, /* 40 */
    {39,17,2,0.3f,100000,18,0.013f,17,90,0.002f,85,false}, /* 41 */
    {40,18,2,0.3f,100000,18,0.013f,17,100,0.004f,85,false}, /* 42 */
    {40,18,2,0.3f,100000,19,0.014f,17,100,0.006f,85,false}, /* 43 */
    {40,18,2,0.3f,100000,19,0.014f,17,100,0.008f,85,false}, /* 44 */
    {40,18,2,0.3f,100000,20,0.015f,17,100,0.010f,90,false}, /* 45 */
    {40,19,2,0.3f,100000,20,0.016f,17,100,0.012f,90,false}, /* 46 */
    {40,19,2,0.3f,100000,20,0.016f,17,100,0.014f,90,false}, /* 47 */
    {40,19,2,0.3f,100000,20,0.017f,17,100,0.016f,90,false}, /* 48 */
    {40,19,2,0.3f,100000,20,0.017f,17,100,0.018f,90,false}, /* 49 */
    {40,20,2,0.3f,100000,20,0.018f,17,100,0.020f,90,false}, /* 50 */
};

/* xorshift32 RNG (same algorithm used in starfield.c) */
static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (*s = x);
}

static float rng_float(uint32_t *s) {
    return (float)(rng_next(s) >> 8) / (float)(1 << 24);
}

static int rng_range(uint32_t *s, int n) {
    return (int)(rng_next(s) % (uint32_t)n);
}

/* True if object o doesn't overlap any of objs[0..count-1] or the ship start. */
static bool obj_no_overlap(const Object *objs, int count, const Object *o, int ship_w, int ship_h) {
    /* 8-pixel clearance around ship start position */
    if (o->xbr + 8 >= SHIP_START_X && o->ybr + 8 >= SHIP_START_Y &&
        o->x  <= SHIP_START_X + ship_w + 8 && o->y <= SHIP_START_Y + ship_h + 8)
        return false;
    for (int i = 0; i < count; i++) {
        if (abs(o->x - objs[i].x) < OBJ_W + 2 &&
            abs(o->y - objs[i].y) < OBJ_H + 2)
            return false;
    }
    return true;
}

void game_init(GameState *g, int sprite_w, int sprite_h, uint32_t rng_seed) {
    Ship *s = &g->ship;
    s->w    = sprite_w;
    s->h    = sprite_h;
    s->x    = SHIP_START_X;
    s->y    = SHIP_START_Y;
    s->xbr  = s->x + s->w - 1;
    s->ybr  = s->y + s->h - 1;
    s->sx   = s->x * 64;
    s->sy   = s->y * 64;
    s->delx = 0;
    s->dely = 0;
    s->dir  = 0;

    g->cam_x = s->x - VIEWPORT_W / 2;
    g->cam_y = s->y - VIEWPORT_H / 2;
    if (g->cam_x < 0) g->cam_x = 0;
    if (g->cam_y < 0) g->cam_y = 0;
    if (g->cam_x > MAX_VISIBLE_X) g->cam_x = MAX_VISIBLE_X;
    if (g->cam_y > MAX_VISIBLE_Y) g->cam_y = MAX_VISIBLE_Y;

    g->level        = 1;
    g->score        = 0;
    g->lives        = START_LIVES;
    g->bombs        = START_BOMBS;
    g->num_crystals = 0;
    g->gate_open    = false;
    g->gate_left_x  = 0;
    g->gate_right_x = 0;
    g->num_objects  = 0;
    g->diff_level     = 2;   /* Average */
    g->gamespeed      = 64;  /* round(64 * speedfactor=1.0) */
    g->rng            = rng_seed ? rng_seed : 0x9E3779B9u;
    g->ship_destroyed     = false;
    g->ship_explode_timer = 0;
    g->frame_count        = 0;
    g->gate_inner_x       = 0;
    g->level_complete     = false;
    g->gate_move_accum    = 0;
    g->gate_move_dir      = 1;
    g->smart_bomb_flash   = 0;
    for (int i = 0; i < PU_COUNT; i++) g->powerup_timer[i] = 0;
    entities_level_init(g);
}

void level_init(GameState *g, int gate_left_w, int gate_right_w) {
    const LevelRecord *rec = &g_levels[g->level - 1];
    (void)gate_right_w;

    /* Gate positions matching Pascal:
         Gate[Left].X  = (PageWidth - GateWidth) div 2 - Gate[Left].Width
         Gate[Right].X = (PageWidth + GateWidth) div 2              */
    g->gate_left_x  = (WORLD_W - rec->gate_width) / 2 - gate_left_w;
    g->gate_right_x = (WORLD_W + rec->gate_width) / 2;

    /* NumSmarts: probabilistic (mirrors Pascal while random < smartprob) */
    int num_smarts = 0;
    while (rng_float(&g->rng) < rec->smartprob && num_smarts < rec->maxsmart)
        num_smarts++;

    int total = rec->numcryst + rec->nummine + num_smarts;
    if (total > MAX_OBJECTS) total = MAX_OBJECTS;

    g->num_objects  = total;
    g->num_crystals = rec->numcryst;
    g->gate_open    = false;
    g->level_complete = false;
    g->gate_inner_x  = (WORLD_W - rec->gate_width) / 2;
    g->gate_move_accum = 0;
    g->gate_move_dir   = 1;
    entities_level_init(g);

    /* Place objects with NoOverlap check (SetupNewLevel in xquest.pas) */
    for (int i = 0; i < total; i++) {
        Object *o = &g->objects[i];
        int attempts = 0;
        do {
            o->x   = MINE_X_MIN + rng_range(&g->rng, MINE_X_MAX - MINE_X_MIN - OBJ_W + 1);
            o->y   = MINE_Y_MIN + rng_range(&g->rng, MINE_Y_MAX - MINE_Y_MIN - OBJ_H + 1);
            o->xbr = o->x + OBJ_W - 1;
            o->ybr = o->y + OBJ_H - 1;
            attempts++;
        } while (attempts < 1000 && !obj_no_overlap(g->objects, i, o, g->ship.w, g->ship.h));

        if (i < rec->numcryst)
            o->type = OBJ_CRYSTAL;
        else if (i < rec->numcryst + rec->nummine)
            o->type = OBJ_MINE;
        else
            o->type = OBJ_SMART;
        o->active = true;
    }
}

void game_tick(GameState *g, int inp_dx, int inp_dy, bool brake) {
    Ship *s = &g->ship;

    s->delx += inp_dx;
    s->dely += inp_dy;

    if (brake) {
        s->delx = (s->delx * 5) / 6;
        s->dely = (s->dely * 5) / 6;
    }

    int speed = abs(s->delx) + abs(s->dely);
    if (speed > MAX_SHIP_SPEED) {
        s->delx = (s->delx * MAX_SHIP_SPEED) / speed;
        s->dely = (s->dely * MAX_SHIP_SPEED) / speed;
    }

    s->sx += s->delx;
    s->sy += s->dely;
    s->x   = s->sx / 64;
    s->y   = s->sy / 64;
    s->xbr = s->x + s->w - 1;
    s->ybr = s->y + s->h - 1;

    /* Wimp difficulty or shield active → bounce off walls.
       All other difficulties → wall contact destroys the ship
       (matches Pascal CheckWallCollisions: DiffLevel > 1 → ShipDestroyed). */
    bool wall_bounce = (g->diff_level == 0) || (g->powerup_timer[PU_SHIELD] > 0);

    if (s->x < SHIP_MIN_X) {
        s->x = SHIP_MIN_X; s->sx = s->x * 64;
        if (wall_bounce) s->delx = abs(s->delx); else g->ship_destroyed = true;
    }
    if (s->xbr > SHIP_MAX_X) {
        s->x = SHIP_MAX_X - s->w; s->sx = s->x * 64;
        if (wall_bounce) s->delx = -abs(s->delx); else g->ship_destroyed = true;
    }
    if (s->y < SHIP_MIN_Y) {
        /* Allow exit through open gate if ship is horizontally inside the opening */
        bool in_gate = g->gate_open
                    && s->xbr >= g->gate_inner_x
                    && s->x   <= g->gate_right_x;
        if (in_gate) {
            if (s->y < 0)
                g->level_complete = true;
        } else {
            s->y = SHIP_MIN_Y; s->sy = s->y * 64;
            if (wall_bounce) s->dely = abs(s->dely); else g->ship_destroyed = true;
        }
    }
    if (s->ybr > SHIP_MAX_Y) {
        s->y = SHIP_MAX_Y - s->h; s->sy = s->y * 64;
        if (wall_bounce) s->dely = -abs(s->dely); else g->ship_destroyed = true;
    }
    s->xbr = s->x + s->w - 1;
    s->ybr = s->y + s->h - 1;

    if (s->delx != 0 || s->dely != 0) {
        double theta = atan2((double)s->delx, (double)s->dely);
        if (theta < 0) theta += 2 * M_PI;
        s->dir = (int)round(theta / (2 * M_PI) * 24) % 24;
    }

    /* Camera scroll (CheckWallCollisions in xquest.pas) */
    int temp = s->x - (g->cam_x + VIEWPORT_W - SCREEN_H_BORDER);
    if (temp > 0 && g->cam_x < MAX_VISIBLE_X) {
        g->cam_x += temp / 20 + 1;
        if (g->cam_x > MAX_VISIBLE_X) g->cam_x = MAX_VISIBLE_X;
    }
    temp = (g->cam_x + SCREEN_H_BORDER) - s->x;
    if (temp > 0 && g->cam_x > 0) {
        g->cam_x -= temp / 20 + 1;
        if (g->cam_x < 0) g->cam_x = 0;
    }
    temp = s->y - (g->cam_y + VIEWPORT_H - SCREEN_V_BORDER);
    if (temp > 0 && g->cam_y < MAX_VISIBLE_Y) {
        g->cam_y += temp / 20 + 1;
        if (g->cam_y > MAX_VISIBLE_Y) g->cam_y = MAX_VISIBLE_Y;
    }
    temp = (g->cam_y + SCREEN_V_BORDER) - s->y;
    if (temp > 0 && g->cam_y > 0) {
        g->cam_y -= temp / 20 + 1;
        if (g->cam_y < 0) g->cam_y = 0;
    }

    /* Moving gate (Pascal MoveGate / GateMoveCount, levels 33+).
       MinGateX=20, MaxGateX=PageWidth-30=362 bound the LEFT gate sprite. */
    const LevelRecord *lrec = &g_levels[g->level - 1];
    if (lrec->gate_move > 0) {
        g->gate_move_accum += lrec->gate_move;
        /* Random direction flip */
        if (rng_float(&g->rng) < lrec->gate_cdir_prob)
            g->gate_move_dir = -g->gate_move_dir;
        /* Move one pixel for each full 64 units accumulated */
        while (g->gate_move_accum >= 64) {
            g->gate_move_accum -= 64;
            g->gate_left_x  += g->gate_move_dir;
            g->gate_right_x += g->gate_move_dir;
            if (g->gate_right_x > 362) g->gate_move_dir = -1;
            if (g->gate_left_x  <  20) g->gate_move_dir =  1;
        }
        g->gate_inner_x = g->gate_left_x + g->ship.w; /* left edge of opening */
    }
}

void level_check_pickups(GameState *g) {
    const Ship *s = &g->ship;
    for (int i = 0; i < g->num_objects; i++) {
        Object *o = &g->objects[i];
        if (!o->active) continue;
        if (s->x > o->xbr || s->xbr < o->x ||
            s->y > o->ybr || s->ybr < o->y)
            continue;
        o->active = false;
        switch (o->type) {
            case OBJ_CRYSTAL:
                audio_play(SND_GETCRYSTAL);
                g->score += 200;
                g->num_crystals--;
                if (g->num_crystals <= 0) {
                    g->num_crystals = 0;
                    g->gate_open    = true;
                    audio_play(SND_GATESOUND);
                }
                break;
            case OBJ_SMART:
                g->bombs++;
                break;
            case OBJ_MINE:
            case OBJ_ENEMY_MINE:
                g->ship_destroyed = true;
                break;
        }
    }
    /* Shield absorbs mine contact */
    if (g->ship_destroyed && g->powerup_timer[PU_SHIELD] > 0)
        g->ship_destroyed = false;
}

void powerups_tick(GameState *g) {
    for (int i = 0; i < PU_COUNT; i++) {
        if (g->powerup_timer[i] > 0)
            g->powerup_timer[i]--;
    }
    /* Smart-bomb flash: decrement every other tick (Pascal FrameCount and 1=0) */
    if (g->smart_bomb_flash > 0 && (g->frame_count & 1) == 0)
        g->smart_bomb_flash--;
}

/* Pascal FireSmartBomb */
void fire_smart_bomb(GameState *g, const Assets *a) {
    if (g->bombs <= 0) return;
    for (int i = g->num_enemies - 1; i >= 0; i--) {
        if (!g->enemies[i].active) continue;
        g->enemies[i].hit = 1;
        enemy_hit(g, a, i, 0, 0);
    }
    g->num_emissiles = 0;
    g->bombs--;
    g->smart_bomb_flash = 11;
    audio_play(SND_EXPLOSN);
}
