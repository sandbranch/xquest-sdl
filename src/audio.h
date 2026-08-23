#pragma once
#include <stdbool.h>

/* Sound indices matching xqvars.pas constants (1-based). */
#define SND_FIRE6       1
#define SND_FIRE5       2
#define SND_PHEW        3
#define SND_FIRE4       4
#define SND_FIRE        5
#define SND_BOING       6
#define SND_SQUELCH     7
#define SND_WOOHOO      8
#define SND_ALLRIGHT    9
#define SND_OHYEAH     10
#define SND_GETCRYSTAL 11
#define SND_EXPLOSN    12
#define SND_EXPLOSN2   13
#define SND_EXPLOSN3   14
#define SND_RETALIATE  15
#define SND_OW         16
#define SND_COUNTDOWN  17
#define SND_GATESOUND  18
#define SND_SXTSMASH   19
#define SND_BARK       20
#define SND_APPLAUSE   21
#define SND_ENEMYENT   22
#define SND_MENUCLICK  23
#define SND_DOH        24
#define SND_REPULSE    25

/* Load xquest.snd and open SDL audio device.
   Returns true on success; false if audio unavailable (game still runs). */
bool audio_init(const char *snd_path);
void audio_free(void);

/* Trigger sound snd (1..25).  No-op if audio not initialised or snd out of range. */
void audio_play(int snd);
