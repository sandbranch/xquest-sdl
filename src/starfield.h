#pragma once
#include <stdint.h>

#define STAR_COUNT   400
#define STAR_SPEED   128
#define STAR_XCENTER 160
#define STAR_YCENTER 120

typedef struct {
    int x, y, z;   /* 3D world position */
    int xz, yz;    /* last projected screen position */
    int c;         /* palette index for drawing (0 = not yet placed) */
} Star;

typedef struct {
    Star     stars[STAR_COUNT];
    uint32_t rng;
} Starfield;

void starfield_init(Starfield *sf, uint32_t seed);
void starfield_step(Starfield *sf);
