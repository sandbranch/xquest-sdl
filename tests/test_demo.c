/* Demo file format and replay determinism.

   The .dmo layout is shared with the 1994 original, so the round-trip test
   uses the reference recording in tests/data and demands the bytes come back
   identical. */
#include "demo.h"
#include "config.h"
#include "game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

static int failures = 0;
#define CHECK(cond, ...) do {                                   \
    if (!(cond)) { printf("FAIL %s:%d: ", __FILE__, __LINE__);  \
                   printf(__VA_ARGS__); printf("\n");           \
                   failures++; }                                \
} while (0)

static int files_equal(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    if (!fa || !fb) { if (fa) fclose(fa); if (fb) fclose(fb); return 0; }
    int same = 1, ca, cb;
    do { ca = fgetc(fa); cb = fgetc(fb); if (ca != cb) { same = 0; break; } }
    while (ca != EOF);
    fclose(fa); fclose(fb);
    return same;
}

/* FNV-1a over the state a replay must reproduce, minus the demo scratch. */
static uint64_t hash_state(const GameState *g) {
    uint64_t h = 1469598103934665603ULL;
    const unsigned char *p = (const unsigned char *)g;
    for (size_t i = 0; i < offsetof(GameState, demo_override); i++) {
        h ^= p[i]; h *= 1099511628211ULL;
    }
    return h;
}

static uint64_t run_track(uint32_t seed, const DemoFrame *frames, int n) {
    GameState g;
    memset(&g, 0, sizeof(g));
    game_init(&g, 16, 16, seed);
    g.diff_level = 2;
    g.gamespeed  = 64;
    level_init(&g, 12, 12);
    for (int i = 0; i < n; i++) {
        g.demo_override = true;
        g.demo_delx = frames[i].delx;
        g.demo_dely = frames[i].dely;
        game_tick(&g, NULL, 0, 0, false);
        level_check_pickups(&g);
        powerups_tick(&g);
    }
    return hash_state(&g);
}

int main(int argc, char **argv) {
    const char *ref = (argc > 1) ? argv[1] : "tests/data/reference.dmo";
    const char *tmp = "test_demo_out.dmo";

    Demo d;
    CHECK(demo_load(&d, ref), "cannot load reference demo %s", ref);
    if (failures) return 1;

    CHECK(d.num_frames > 0, "reference demo has no frames");
    CHECK(demo_difficulty(&d) >= 0 && demo_difficulty(&d) <= 4,
          "difficulty %d out of range (offset wrong?)", demo_difficulty(&d));

    /* Round-trip must be byte-exact: the format is shared with the original. */
    CHECK(demo_save(&d, tmp), "demo_save failed");
    CHECK(files_equal(ref, tmp), "round-trip is not byte-identical");

    /* Replay determinism: same seed and track give the same state; the seed
       matters; and the track really does drive the ship. */
    int n = d.num_frames < 3000 ? d.num_frames : 3000;
    uint64_t a = run_track(12345, d.frames, n);
    uint64_t b = run_track(12345, d.frames, n);
    CHECK(a == b, "replay is not deterministic: %016llx vs %016llx",
          (unsigned long long)a, (unsigned long long)b);

    uint64_t c = run_track(99999, d.frames, n);
    CHECK(c != a, "different seeds produced identical state");

    DemoFrame *mod = malloc((size_t)n * sizeof(DemoFrame));
    memcpy(mod, d.frames, (size_t)n * sizeof(DemoFrame));
    mod[n / 2].delx += 3;
    uint64_t e = run_track(12345, mod, n);
    CHECK(e != a, "perturbing a frame changed nothing: track is not driving the ship");
    free(mod);

    /* An empty or truncated file must fail cleanly, not crash. */
    FILE *f = fopen(tmp, "wb"); fclose(f);
    Demo empty;
    CHECK(!demo_load(&empty, tmp), "empty file should fail to load");

    demo_free(&d);
    remove(tmp);
    printf("%s: %d failure(s)\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
