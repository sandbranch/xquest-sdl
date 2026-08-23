#pragma once
#include <stdbool.h>

#define HI_NUM_DIFF    5
#define HI_NUM_SCORES 10
#define HI_NAME_MAX   20

typedef struct {
    long score;                 /* int32 in original (Pascal longint) */
    int  level;                 /* word in original */
    char name[HI_NAME_MAX + 1]; /* NUL-terminated C string */
} HiEntry;

typedef struct {
    HiEntry table[HI_NUM_DIFF][HI_NUM_SCORES]; /* [diff 0-4][rank 0-9] */
} HiTable;

/* Load from path. Returns false and fills with defaults on failure. */
bool hi_load(HiTable *ht, const char *path);

/* Save to path. Returns false on error. */
bool hi_save(const HiTable *ht, const char *path);

/* Return 0-based rank if score qualifies for diff table; -1 if not top 10. */
int  hi_check(const HiTable *ht, int diff, long score);

/* Insert score/level/name at rank, shifting lower entries down. */
void hi_insert(HiTable *ht, int diff, int rank, long score, int level,
               const char *name);
