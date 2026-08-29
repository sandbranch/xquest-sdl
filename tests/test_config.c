/* xquest.cfg round-trip. The file format is shared with the 1994 original,
   so these tests pin the exact bytes, not just our own read/write agreeing
   with each other. */
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(cond, ...) do {                                   \
    if (!(cond)) { printf("FAIL %s:%d: ", __FILE__, __LINE__);  \
                   printf(__VA_ARGS__); printf("\n");           \
                   failures++; }                                \
} while (0)

/* Byte-for-byte what the original shipped in distrib/xquest.cfg. */
static const char *EXPECTED_DEFAULTS =
    "  24 Sound Volume\r\n"
    "   1 Number of Players\r\n"
    "\r\n"
    "Player One\r\n"
    "  64 Horizontal Input Sensitivity\r\n"
    "  64 Vertical Input Sensitivity\r\n"
    "   2 Difficulty Level\r\n"
    "   0 InputDevice\r\n"
    "   1 Mouse Fire Button\r\n"
    "   2 Mouse Smartbomb Button\r\n"
    "  16 Joystick Fire Button\r\n"
    "  32 Joystick Smartbomb Button\r\n"
    "200 208 203 205 71 73 79 81 76 57 28  Keys\r\n"
    "\r\n"
    "Player Two\r\n"
    "  64 Horizontal Input Sensitivity\r\n"
    "  64 Vertical Input Sensitivity\r\n"
    "   2 Difficulty Level\r\n"
    "   0 InputDevice\r\n"
    "   1 Mouse Fire Button\r\n"
    "   2 Mouse Smartbomb Button\r\n"
    "  16 Joystick Fire Button\r\n"
    "  32 Joystick Smartbomb Button\r\n"
    "72 80 75 77 71 73 79 81 76 28 57  Keys\r\n"
    "\r\n"
    "8 30973 2686 0 0 0 0 0 Joystick calibration values\r\n"
    "   0 Joystick calibrated?\r\n"
    "   0 Sound card\r\n"
    " 544 Port\r\n"
    "   5 IRQ\r\n"
    "   1 DMA\r\n"
    "   8 Maximum simultaneous sounds\r\n";

static char *slurp(const char *path, size_t *len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fclose(f); free(buf); return NULL; }
    fclose(f);
    buf[n] = '\0';
    *len = (size_t)n;
    return buf;
}

int main(void) {
    const char *tmp = "test_config_out.cfg";

    /* Defaults must serialise to the original's exact bytes. */
    Config c;
    config_defaults(&c);
    CHECK(config_save(&c, tmp), "config_save failed");
    size_t n = 0;
    char *got = slurp(tmp, &n);
    CHECK(got != NULL, "could not read back written config");
    if (got) {
        CHECK(n == strlen(EXPECTED_DEFAULTS),
              "length %zu, expected %zu", n, strlen(EXPECTED_DEFAULTS));
        CHECK(got && strcmp(got, EXPECTED_DEFAULTS) == 0,
              "written config does not match the original byte for byte");
        free(got);
    }

    /* A changed setting survives a save/load cycle. */
    c.player[0].difficulty = 4;
    c.sound_volume = 77;
    CHECK(config_save(&c, tmp), "config_save failed");
    Config back;
    CHECK(config_load(&back, tmp), "config_load failed");
    CHECK(back.player[0].difficulty == 4, "difficulty %d, expected 4",
          back.player[0].difficulty);
    CHECK(back.sound_volume == 77, "volume %d, expected 77", back.sound_volume);

    /* Fields we have no UI for must survive untouched. */
    CHECK(back.sb_addr == 544 && back.sb_irq == 5 && back.sb_dma == 1,
          "sound hardware fields not preserved");
    CHECK(back.player[1].keys[0] == 72, "player 2 keys not preserved");

    /* A missing file yields defaults and reports failure. */
    Config miss;
    CHECK(!config_load(&miss, "no/such/file.cfg"), "missing file should fail");
    CHECK(miss.player[0].difficulty == 2, "missing file should give defaults");

    /* Out-of-range values are clamped rather than trusted. */
    FILE *f = fopen(tmp, "wb");
    fprintf(f, "9999 Sound Volume\r\n 7 Number of Players\r\n\r\nPlayer One\r\n"
               "64 h\r\n64 v\r\n 99 Difficulty Level\r\n0 d\r\n1 a\r\n2 b\r\n0 c\r\n1 e\r\n"
               "1 2 3 4 5 6 7 8 9 10 11 Keys\r\n\r\nPlayer Two\r\n"
               "64 h\r\n64 v\r\n2 Difficulty Level\r\n0 d\r\n1 a\r\n2 b\r\n0 c\r\n1 e\r\n"
               "1 2 3 4 5 6 7 8 9 10 11 Keys\r\n");
    fclose(f);
    Config bad;
    config_load(&bad, tmp);
    CHECK(bad.sound_volume == 128, "volume %d, expected clamp to 128", bad.sound_volume);
    CHECK(bad.num_players == 1, "players %d, expected clamp to 1", bad.num_players);
    CHECK(bad.player[0].difficulty == 2, "difficulty %d, expected clamp to 2",
          bad.player[0].difficulty);

    remove(tmp);
    printf("%s: %d failure(s)\n", failures ? "FAILED" : "ok", failures);
    return failures != 0;
}
