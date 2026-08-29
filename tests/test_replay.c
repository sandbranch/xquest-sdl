/* End-to-end replay regression.

   Replays a recorded demo through the real engine and checks the run comes
   out exactly as it did when the demo was recorded. This is the test that
   catches physics and RNG changes: a demo encodes a player's inputs, so
   everything else about the run is the engine's doing.

   The tick loop below must mirror main.c. In particular the demo frame index
   advances only on ticked frames, never during the explosion freeze; getting
   that wrong silently desynchronises the replay after the first death.

   If this fails after a deliberate gameplay change, the recording is stale
   rather than wrong: re-record with --record and update the expectations.

   Coverage is bounded by what the recording actually does. This one never
   flies near the bottom wall, for instance, so it would not notice that
   boundary moving. Adding further recordings widens the net. */
#include "demo.h"
#include "game.h"
#include "assets.h"
#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond, ...) do {                                   \
    if (!(cond)) { printf("FAIL %s:%d: ", __FILE__, __LINE__);  \
                   printf(__VA_ARGS__); printf("\n");           \
                   failures++; }                                \
} while (0)

/* Expected outcome of tests/data/reference.dmo on the current engine. */
#define EXPECT_FRAMES   12066
#define EXPECT_LEVELS       6
#define EXPECT_CRYSTALS   162
#define EXPECT_SCORE    36000
#define EXPECT_DEATHS       2

int main(int argc, char **argv) {
    const char *asset_dir = (argc > 1) ? argv[1] : "assets";
    const char *demo_path = (argc > 2) ? argv[2] : "tests/data/reference.dmo";

    Assets a;
    if (assets_load(&a, asset_dir) != 0) {
        printf("FAIL: assets_load(%s)\n", asset_dir);
        return 1;
    }
    Demo d;
    if (!demo_load(&d, demo_path)) {
        printf("FAIL: demo_load(%s)\n", demo_path);
        return 1;
    }

    static const int diff_speed[5] = {45, 54, 64, 77, 96};
    int diff = demo_difficulty(&d);
    if (diff < 0 || diff > 4) diff = 2;

    GameState g;
    memset(&g, 0, sizeof(g));
    game_init(&g, a.ship[0].w, a.ship[0].h, d.seed);
    g.diff_level = diff;
    g.gamespeed  = diff_speed[diff];
    level_init(&g, a.gate_left.w, a.gate_right.w);

    int ptr = 0, deaths = 0, levels = 0, crystals = 0;
    bool game_over = false;

    while (ptr < d.num_frames && !game_over) {
        bool exploding = (g.ship_explode_timer > 0);
        if (!exploding) {
            const DemoFrame *fr = &d.frames[ptr];
            g.demo_override = true;
            g.demo_delx = fr->delx;
            g.demo_dely = fr->dely;

            bool wf = (fr->but & DEMO_BUT_FIRE) != 0;
            bool wh = (fr->but & DEMO_BUT_FIRE_HELD) != 0;
            if (wf) shoot(&g, &a);
            if (g.powerup_timer[PU_RAPID] > 0 && !wf && wh &&
                (g.frame_count & 3) == 0)
                shoot(&g, &a);
            if (fr->but & DEMO_BUT_BOMB) fire_smart_bomb(&g, &a);

            int before = g.num_crystals;
            game_tick(&g, &a, 0, 0, false);
            ptr++;
            level_check_pickups(&g);
            powerups_tick(&g);
            if (g.num_crystals < before) crystals += before - g.num_crystals;
        }
        missiles_tick(&g, &a);
        entities_tick(&g, &a);
        emissiles_tick(&g, &a);
        if (!exploding) check_collisions(&g, &a);

        if (g.level_complete) {
            levels++;
            uint32_t rng = g.rng; long sc = g.score;
            int lv = g.level + 1, li = g.lives, bo = g.bombs;
            if (lv > MAX_LEVELS) break;
            game_init(&g, a.ship[0].w, a.ship[0].h, rng);
            g.score = sc; g.level = lv; g.lives = li; g.bombs = bo;
            g.diff_level = diff; g.gamespeed = diff_speed[diff];
            level_init(&g, a.gate_left.w, a.gate_right.w);
            continue;
        }
        if (g.ship_destroyed) { g.ship_destroyed = false; ship_destroy(&g, &a); }
        if (g.ship_explode_timer > 0 && --g.ship_explode_timer == 0) {
            deaths++;
            g.lives--;
            if (g.lives <= 0) { game_over = true; break; }
            uint32_t rng = g.rng; long sc = g.score;
            int lv = g.level, li = g.lives, bo = g.bombs;
            game_init(&g, a.ship[0].w, a.ship[0].h, rng);
            g.score = sc; g.level = lv; g.lives = li; g.bombs = bo;
            g.diff_level = diff; g.gamespeed = diff_speed[diff];
            level_init(&g, a.gate_left.w, a.gate_right.w);
        }
    }

    printf("replayed %d/%d frames, %d levels, %d crystals, %ld points, %d deaths\n",
           ptr, d.num_frames, levels, crystals, g.score, deaths);

    CHECK(ptr == EXPECT_FRAMES, "used %d frames, expected %d (replay diverged)",
          ptr, EXPECT_FRAMES);
    CHECK(!game_over, "run ended in game over; the recording plays to the end");
    CHECK(levels   == EXPECT_LEVELS,   "levels %d, expected %d", levels, EXPECT_LEVELS);
    CHECK(crystals == EXPECT_CRYSTALS, "crystals %d, expected %d", crystals, EXPECT_CRYSTALS);
    CHECK(g.score  == EXPECT_SCORE,    "score %ld, expected %d", g.score, EXPECT_SCORE);
    CHECK(deaths   == EXPECT_DEATHS,   "deaths %d, expected %d", deaths, EXPECT_DEATHS);

    demo_free(&d);
    printf("%s: %d failure(s)\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
