#include "audio.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* xquest.snd: raw 8-bit unsigned PCM, 11025 Hz mono.
   File layout: for each of 25 sounds → [uint16_t length][length bytes of PCM]. */
#define AUDIO_RATE    11025
#define MAX_SOUNDS       25
#define MIX_CHANNELS      8   /* simultaneous voices */

typedef struct {
    uint8_t  *data;
    uint32_t  len;
    int       vol;   /* 0–128; applied per-channel before summing */
} Sample;

typedef struct {
    const uint8_t *data;
    uint32_t       len;
    uint32_t       pos;
    int            vol;
} Channel;

static Sample           g_samples[MAX_SOUNDS + 1]; /* 1-indexed */
static Channel          g_ch[MIX_CHANNELS];
static SDL_AudioDeviceID g_dev = 0;
static bool             g_ready = false;

/* SDL audio callback - runs on a separate thread.
   Mixes active channels: each U8 sample is centred at 128, summed, scaled to S16. */
static void audio_cb(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *out     = (int16_t *)stream;
    int      samples = len / 2;

    for (int i = 0; i < samples; i++) {
        int32_t mix = 0;
        for (int c = 0; c < MIX_CHANNELS; c++) {
            if (g_ch[c].pos < g_ch[c].len)
                mix += ((int32_t)g_ch[c].data[g_ch[c].pos++] - 128) * g_ch[c].vol;
        }
        /* Scale: vol is 0-128, so mix is 128x larger than raw amplitude sum.
           350 / 128 ≈ 2.7 - single full-amplitude channel ≈ 43 % of S16 max. */
        mix = mix * 350 / 128;
        if (mix >  32767) mix =  32767;
        if (mix < -32768) mix = -32768;
        out[i] = (int16_t)mix;
    }
}

bool audio_init(const char *snd_path) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "audio: SDL_InitSubSystem: %s\n", SDL_GetError());
        return false;
    }

    /* Load samples from xquest.snd */
    FILE *f = fopen(snd_path, "rb");
    if (!f) {
        perror(snd_path);
        return false;
    }
    for (int i = 1; i <= MAX_SOUNDS; i++) {
        uint8_t lb[2];
        if (fread(lb, 1, 2, f) != 2) break;
        uint32_t len = lb[0] | ((uint32_t)lb[1] << 8);
        g_samples[i].data = malloc(len);
        g_samples[i].len  = len;
        g_samples[i].vol  = 128;
        if (!g_samples[i].data || fread(g_samples[i].data, 1, len, f) != len) {
            fprintf(stderr, "audio: failed reading sound %d\n", i);
            fclose(f);
            return false;
        }
    }
    fclose(f);

    /* Per-sound volume tweaks (0-128). */
    g_samples[SND_GETCRYSTAL].vol = 45;   /* glass-shatter quieter */

    SDL_AudioSpec want, got;
    SDL_zero(want);
    want.freq     = AUDIO_RATE;
    want.format   = AUDIO_S16SYS;
    want.channels = 1;
    want.samples  = 512;
    want.callback = audio_cb;

    g_dev = SDL_OpenAudioDevice(NULL, 0, &want, &got, 0);
    if (!g_dev) {
        fprintf(stderr, "audio: SDL_OpenAudioDevice: %s\n", SDL_GetError());
        return false;
    }
    if (got.format != AUDIO_S16SYS || got.freq != AUDIO_RATE) {
        fprintf(stderr, "audio: device opened at wrong format/rate (wanted S16 @ %d, got fmt=%d @ %d)\n",
                AUDIO_RATE, got.format, got.freq);
        SDL_CloseAudioDevice(g_dev);
        g_dev = 0;
        return false;
    }

    SDL_PauseAudioDevice(g_dev, 0);
    g_ready = true;
    return true;
}

void audio_free(void) {
    if (g_dev) { SDL_CloseAudioDevice(g_dev); g_dev = 0; }
    g_ready = false;
    for (int i = 1; i <= MAX_SOUNDS; i++) {
        free(g_samples[i].data);
        g_samples[i].data = NULL;
    }
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

void audio_play(int snd) {
    if (!g_ready || snd < 1 || snd > MAX_SOUNDS) return;
    if (!g_samples[snd].data) return;

    SDL_LockAudioDevice(g_dev);

    /* Find a free channel; if none, reuse the one furthest through its sample. */
    int best = 0;
    uint32_t best_pos = 0;
    for (int c = 0; c < MIX_CHANNELS; c++) {
        if (g_ch[c].pos >= g_ch[c].len) { best = c; break; }   /* free */
        if (g_ch[c].pos > best_pos) { best_pos = g_ch[c].pos; best = c; }
    }
    g_ch[best].data = g_samples[snd].data;
    g_ch[best].len  = g_samples[snd].len;
    g_ch[best].pos  = 0;
    g_ch[best].vol  = g_samples[snd].vol;

    SDL_UnlockAudioDevice(g_dev);
}
