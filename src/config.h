#pragma once
#include <stdbool.h>
#include <stddef.h>

/* Reads and writes the original XQuest xquest.cfg.

   Despite its name the original config is a plain *text* file with CRLF line
   endings, one "value label" line per setting (see WriteDefaults in
   xquest.pas). We keep that layout byte-for-byte so a file written here still
   loads in the DOS original, and vice versa.

   Fields the SDL port has no UI for (sensitivity, key bindings, joystick
   calibration, Sound Blaster port/IRQ/DMA) are still parsed and written back
   unchanged, so round-tripping someone's original file never loses settings. */

#define CFG_NUM_PLAYERS  2   /* the file always stores both player slots */
#define CFG_NUM_KEYS    11   /* UpKey..SmartBombKey */

typedef struct {
    int h_sensitivity;       /* 0-128 */
    int v_sensitivity;
    int difficulty;          /* 0=Wimp .. 4=Inhuman, 2=Average */
    int input_device;        /* ord(InputDeviceType): 0=Mouse, 1=Joystick, 2=Keyboard */
    int mouse_fire, mouse_smart;
    int joy_fire,   joy_smart;
    int keys[CFG_NUM_KEYS];  /* DOS scancodes; preserved verbatim, see note above */
} CfgPlayer;

typedef struct {
    int sound_volume;        /* 0-128 */
    int num_players;         /* 1 or 2 */
    CfgPlayer player[CFG_NUM_PLAYERS];

    int  joy_cal[8];         /* XMin, XCentreMin, XCentreMax, XMax, then Y */
    bool joy_calibrated;

    /* DOS-era sound hardware. Meaningless under SDL, kept for file fidelity. */
    int sound_card, sb_addr, sb_irq, sb_dma, max_sound_effects;
} Config;

/* Fill cfg with the factory defaults shipped in the original distrib/xquest.cfg. */
void config_defaults(Config *cfg);

/* Resolve where xquest.cfg should live and store it in buf.

   Order: $XQUEST_CONFIG_DIR, then a writable xquest.cfg sitting next to the
   game data (portable or original DOS install), then the per-user data dir
   from SDL_GetPrefPath. The last is the normal case for an installed build,
   whose asset dir is read-only. */
void config_path(char *buf, size_t n, const char *asset_dir);

/* Load from path. On any failure cfg is left holding defaults; returns false. */
bool config_load(Config *cfg, const char *path);

/* Write cfg to path in the original format. Returns false on error. */
bool config_save(const Config *cfg, const char *path);
