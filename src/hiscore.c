#include "hiscore.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

/* Each entry: 4 bytes score (int32 LE) + 2 bytes level (uint16 LE)
   + 1 byte Pascal string length + 20 bytes name = 27 bytes */
#define ENTRY_BYTES 27

static void fill_defaults(HiTable *ht) {
    for (int d = 0; d < HI_NUM_DIFF; d++) {
        for (int i = 0; i < HI_NUM_SCORES; i++) {
            ht->table[d][i].score = 0;
            ht->table[d][i].level = 1;
            strncpy(ht->table[d][i].name, "XQuest", HI_NAME_MAX);
            ht->table[d][i].name[HI_NAME_MAX] = '\0';
        }
    }
}

bool hi_load(HiTable *ht, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fill_defaults(ht); return false; }

    for (int d = 0; d < HI_NUM_DIFF; d++) {
        for (int i = 0; i < HI_NUM_SCORES; i++) {
            uint8_t buf[ENTRY_BYTES];
            if (fread(buf, 1, ENTRY_BYTES, f) != ENTRY_BYTES) {
                fclose(f);
                fill_defaults(ht);
                return false;
            }
            /* score: int32 LE at [0..3] */
            int32_t sc = (int32_t)( (uint32_t)buf[0]
                                  | ((uint32_t)buf[1] << 8)
                                  | ((uint32_t)buf[2] << 16)
                                  | ((uint32_t)buf[3] << 24));
            ht->table[d][i].score = (long)sc;
            /* level: uint16 LE at [4..5] */
            ht->table[d][i].level = (int)(buf[4] | ((unsigned)buf[5] << 8));
            /* name: Pascal string at [6]: 1-byte len + chars */
            int len = buf[6];
            if (len > HI_NAME_MAX) len = HI_NAME_MAX;
            memcpy(ht->table[d][i].name, buf + 7, len);
            ht->table[d][i].name[len] = '\0';
        }
    }
    fclose(f);
    return true;
}

bool hi_save(const HiTable *ht, const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    for (int d = 0; d < HI_NUM_DIFF; d++) {
        for (int i = 0; i < HI_NUM_SCORES; i++) {
            uint8_t buf[ENTRY_BYTES];
            memset(buf, 0, ENTRY_BYTES);

            int32_t sc = (int32_t)ht->table[d][i].score;
            buf[0] = (uint8_t)( sc        & 0xFF);
            buf[1] = (uint8_t)((sc >>  8) & 0xFF);
            buf[2] = (uint8_t)((sc >> 16) & 0xFF);
            buf[3] = (uint8_t)((sc >> 24) & 0xFF);

            uint16_t lv = (uint16_t)ht->table[d][i].level;
            buf[4] = (uint8_t)(lv       & 0xFF);
            buf[5] = (uint8_t)((lv >> 8) & 0xFF);

            int len = (int)strlen(ht->table[d][i].name);
            if (len > HI_NAME_MAX) len = HI_NAME_MAX;
            buf[6] = (uint8_t)len;
            memcpy(buf + 7, ht->table[d][i].name, len);

            if (fwrite(buf, 1, ENTRY_BYTES, f) != ENTRY_BYTES) {
                fclose(f);
                return false;
            }
        }
    }
    fclose(f);
    return true;
}

int hi_check(const HiTable *ht, int diff, long score) {
    if (diff < 0 || diff >= HI_NUM_DIFF || score <= 0) return -1;
    for (int i = 0; i < HI_NUM_SCORES; i++) {
        if (score > ht->table[diff][i].score)
            return i;
    }
    return -1;
}

void hi_insert(HiTable *ht, int diff, int rank, long score, int level,
               const char *name) {
    for (int i = HI_NUM_SCORES - 1; i > rank; i--)
        ht->table[diff][i] = ht->table[diff][i - 1];

    ht->table[diff][rank].score = score;
    ht->table[diff][rank].level = level;
    int len = (int)strlen(name);
    if (len > HI_NAME_MAX) len = HI_NAME_MAX;
    memcpy(ht->table[diff][rank].name, name, len);
    ht->table[diff][rank].name[len] = '\0';
}
