#include "starfield.h"

/* xorshift32 - fast, good quality for cosmetic use */
static uint32_t rng_next(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (*s = x);
}

static int rng_range(uint32_t *s, int n) {
    return (int)(rng_next(s) % (uint32_t)n);
}

/* initstar: scatter star at a random depth in the field (init only) */
static void initstar(Star *st, uint32_t *rng) {
    st->x  = -5000 + rng_range(rng, 10000);
    st->y  = -5000 + rng_range(rng, 10000);
    st->z  = rng_range(rng, 12000) + 256;
    st->xz = 1;
    st->yz = 1;
    st->c  = 0;
}

/* newstar: recycle star at the far end of the field */
static void newstar(Star *st, uint32_t *rng) {
    st->x  = -8191 + rng_range(rng, 16383);
    st->y  = -8191 + rng_range(rng, 16383);
    st->z  = rng_range(rng, 1256) + 14500;
    st->xz = 1;
    st->yz = 1;
    st->c  = 0;
}

void starfield_init(Starfield *sf, uint32_t seed) {
    sf->rng = seed ? seed : 0x9E3779B9u;
    for (int i = 0; i < STAR_COUNT; i++)
        initstar(&sf->stars[i], &sf->rng);
}

void starfield_step(Starfield *sf) {
    for (int i = 0; i < STAR_COUNT; i++) {
        Star *st = &sf->stars[i];

        /* Recycle if previous screen position was off-screen (mirrors
           the bounds check at the top of the assembler update routine) */
        if (st->xz < 0 || st->xz >= 320 || st->yz < 0 || st->yz >= 240) {
            newstar(st, &sf->rng);
            continue;
        }

        /* Move star toward viewer */
        st->z -= STAR_SPEED;
        if (st->z < 257) {
            newstar(st, &sf->rng);
            continue;
        }

        /* Perspective projection: divide by hi(z) = z >> 8 */
        int hi_z = st->z >> 8;
        st->xz = st->x / hi_z + STAR_XCENTER;
        st->yz = st->y / hi_z + STAR_YCENTER;

        /* Color: closer = brighter; palette[1..31] is the grayscale ramp */
        st->c = 31 - (st->z >> 9);
        if (st->c < 1) st->c = 1;
    }
}
