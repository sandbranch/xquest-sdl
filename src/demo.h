#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "config.h"

/* Record and play back demos in the original xquest.dmo format.

   File layout (from SetDemoFile / MoveShip in xquest.pas):
     offset 0   uint32  saved random seed
     offset 4   uint8   GameMode (0 = one player, 1 = two player)
     offset 5   74      PlayerInfoType: two 37-byte player records
     offset 79  ...     frames, 5 bytes each: int16 delx, int16 dely, uint8 but

   Each frame stores the ship's velocity for that tick, captured after braking
   but before the speed clamp, rather than raw input. That makes a recording
   independent of input device and sensitivity.

   Compatibility note: this reads and writes the original layout, but the 1994
   xquest.dmo will not *replay* correctly here. Playback only reproduces the
   original run if the random number stream matches frame for frame, and the
   port uses xorshift32 where Turbo Pascal used its own LCG. The ship would
   fly its recorded path through a differently populated level. Demos this
   port records replay exactly. */

#define DEMO_BLOCK_FRAMES 13100  /* MaxDemoFrames: the file's block size */
#define DEMO_HEADER_BYTES 79
#define DEMO_FRAME_BYTES   5
#define DEMO_PLAYERINFO_BYTES 74

/* but bits, as documented in xquest.pas */
#define DEMO_BUT_FIRE      1   /* missile button pressed */
#define DEMO_BUT_BOMB      2   /* smart bomb button pressed */
#define DEMO_BUT_FIRE_HELD 4   /* missile button held (rapid fire) */

typedef struct {
    int16_t delx, dely;
    uint8_t but;
} DemoFrame;

typedef struct {
    uint32_t   seed;
    uint8_t    game_mode;
    uint8_t    player_info[DEMO_PLAYERINFO_BYTES];
    DemoFrame *frames;
    int        num_frames;
    int        cap;
} Demo;

/* Start an empty recording. player_info is built from cfg. */
void demo_start(Demo *d, uint32_t seed, const Config *cfg);

/* Append one frame. Returns false only if memory ran out. */
bool demo_append(Demo *d, int delx, int dely, uint8_t but);

/* Load/save the original format. */
bool demo_load(Demo *d, const char *path);
bool demo_save(const Demo *d, const char *path);

void demo_free(Demo *d);

/* Difficulty the demo was recorded at (player 1). */
int demo_difficulty(const Demo *d);
