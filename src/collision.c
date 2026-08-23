#include "game.h"
#include <stdbool.h>
#include <stdint.h>

static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s; x ^= x << 13; x ^= x >> 17; x ^= x << 5; return (*s = x);
}

/* Pixel-accurate bitmask collision between two sprites.
   Bit layout: bit 31 = leftmost pixel (column 0).
   Caller guarantees x1 <= x2 and shift = x2-x1 in [0, 31].
   Algorithm mirrors the original x86 asm in xquest.pas line ~1201:
   (mask1_row << shift) & mask2_row - left-shifting mask1 moves its pixels
   rightward on screen to align with mask2 at x2. */
static bool collide_bitmaps(const uint32_t *m1, int h1, int x1, int y1,
                             const uint32_t *m2, int h2, int x2, int y2) {
    if (!m1 || !m2) return false;
    int shift = x2 - x1;
    if (shift < 0 || shift >= 32) return false;
    int ys = y2 > y1 ? y2 : y1;
    int ye = (y1 + h1) < (y2 + h2) ? (y1 + h1) : (y2 + h2);
    for (int y = ys; y < ye; y++) {
        if ((m1[y - y1] << shift) & m2[y - y2]) return true;
    }
    return false;
}

static bool sprites_collide(const Sprite *s1, int x1, int y1,
                             const Sprite *s2, int x2, int y2) {
    if (x1 <= x2)
        return collide_bitmaps(s1->mask, s1->h, x1, y1,
                               s2->mask, s2->h, x2, y2);
    else
        return collide_bitmaps(s2->mask, s2->h, x2, y2,
                               s1->mask, s1->h, x1, y1);
}

void check_collisions(GameState *g, const Assets *a) {
    /* Player missiles vs enemies */
    for (int i = 0; i < g->num_missiles; i++) {
        Missile *m = &g->missiles[i];
        if (!m->active) continue;
        const Sprite *ms = &a->player_missile;

        for (int j = g->num_enemies - 1; j >= 0; j--) {
            Enemy *e = &g->enemies[j];
            if (!e->active || e->type_idx == 1) continue;

            const Sprite *es = &a->enemy[e->type_idx][e->frame >> 8];
            if (!es->px) continue;

            /* Bounding box pre-check */
            if (m->xbr < e->x || m->x > e->xbr || m->ybr < e->y || m->y > e->ybr) continue;

            if (sprites_collide(ms, m->x, m->y, es, e->x, e->y)) {
                enemy_hit(g, a, j, m->delx, m->dely);
                m->active = false;
                break;
            }
        }
    }

    if (g->ship_destroyed) return;

    /* Ship vs enemies */
    const Sprite *ship_sp = &a->ship[g->ship.dir];
    for (int j = g->num_enemies - 1; j >= 0; j--) {
        Enemy *e = &g->enemies[j];
        if (!e->active || e->type_idx == 1) continue;

        const Sprite *es = &a->enemy[e->type_idx][e->frame >> 8];
        if (!es->px) continue;

        if (g->ship.xbr < e->x || g->ship.x > e->xbr ||
            g->ship.ybr < e->y || g->ship.y > e->ybr) continue;

        if (sprites_collide(ship_sp, g->ship.x, g->ship.y, es, e->x, e->y)) {
            if (e->type_idx == 0) {
                /* Supercrystal: grant a powerup (Pascal repeat..until SpecialGiven) */
                e->active = false;
                static const int time_min[PU_COUNT] = {
                    PU_TIMEMIN_SHIELD, PU_TIMEMIN_AIMED, PU_TIMEMIN_LONG,
                    PU_TIMEMIN_LONG,   PU_TIMEMIN_LONG,  PU_TIMEMIN_LONG,
                    PU_TIMEMIN_BOUNCE
                };
                static const int time_ran[PU_COUNT] = {
                    PU_TIMRAN_SHIELD, PU_TIMRAN_AIMED, PU_TIMRAN_LONG,
                    PU_TIMRAN_LONG,   PU_TIMRAN_LONG,  PU_TIMRAN_LONG,
                    PU_TIMRAN_BOUNCE
                };
                /* Maps random(22) → powerup index; -1 = special case */
                static const int roll_to_pu[22] = {
                    PU_RAPID, PU_RAPID, PU_RAPID,        /* 0-2  */
                    PU_MULTI, PU_MULTI, PU_MULTI,        /* 3-5  */
                    PU_HEAVY, PU_HEAVY, PU_HEAVY,        /* 6-8  */
                    PU_ASS,   PU_ASS,   PU_ASS,          /* 9-11 */
                    PU_AIMED, PU_AIMED,                  /* 12-13 */
                    PU_BOUNCE, PU_BOUNCE,                /* 14-15 */
                    -1, -1,                              /* 16-17: clear mines */
                    PU_SHIELD,                           /* 18   */
                    -2, -2, -2                           /* 19-21: gate freeze (skip) */
                };
                for (int tries = 0; tries < 100; tries++) {
                    int roll = (int)(rng_next(&g->rng) % 22u);
                    int pu = roll_to_pu[roll];
                    if (pu >= 0) {
                        if (pu == PU_SHIELD || g->powerup_timer[pu] <= 0) {
                            g->powerup_timer[pu] += (int)(rng_next(&g->rng) % (unsigned)time_ran[pu]) + time_min[pu];
                            break;
                        }
                    } else if (pu == -1) {
                        /* Clear all mines */
                        if (g->powerup_timer[PU_SHIELD] == 0) g->powerup_timer[PU_SHIELD] = 1;
                        for (int oi = 0; oi < g->num_objects; oi++)
                            if (g->objects[oi].active && g->objects[oi].type == OBJ_MINE)
                                g->objects[oi].active = false;
                        break;
                    } else {
                        break;  /* gate freeze: skip silently */
                    }
                }
            } else {
                enemy_hit(g, a, j, g->ship.delx, g->ship.dely);
                if (g->powerup_timer[PU_SHIELD] > 0)
                    g->ship_destroyed = false;
                else
                    g->ship_destroyed = true;
            }
            break;
        }
    }

    if (g->ship_destroyed) return;

    /* Enemy missiles vs ship */
    for (int i = 0; i < g->num_emissiles; i++) {
        EnemyMissile *em = &g->emissiles[i];
        if (!em->active) continue;
        const Sprite *ems = &a->enemy_missile[em->firetype - 1];
        if (!ems->px) continue;

        if (g->ship.xbr < em->x || g->ship.x > em->xbr ||
            g->ship.ybr < em->y || g->ship.y > em->ybr) continue;

        if (sprites_collide(ship_sp, g->ship.x, g->ship.y, ems, em->x, em->y)) {
            em->active = false;
            if (g->powerup_timer[PU_SHIELD] <= 0)
                g->ship_destroyed = true;
            break;
        }
    }
}
