#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assets.h"
#include "render.h"
#include "input.h"
#include "game.h"
#include "menu.h"
#include "hiscore.h"
#include "starfield.h"
#include "audio.h"
#include "config.h"
#include "demo.h"

#ifndef ASSET_DIR
#define ASSET_DIR "../xquest"
#endif

#define TICK_MS 15   /* ~67 fps fixed timestep */

/* The game renders 320x240 and the window is always a whole multiple of
   that, so every game pixel stays a square block. A fixed 3x window is a
   postage stamp on a 4K panel, so the default fits the window to the
   display; --scale / XQUEST_SCALE override it. The sizing itself lives in
   render.c, next to the hotkeys that resize the window at runtime. */

/* Parse a scale override: a number, or 0 for "auto". Returns -1 if it is
   neither, which is distinct from 0 because asking for auto is a real
   choice that overrides the saved size. */
static int parse_scale(const char *s) {
    if (SDL_strcasecmp(s, "auto") == 0) return 0;
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < SCALE_MIN || v > SCALE_MAX) return -1;
    return (int)v;
}

/* Settings are a convenience, never a reason to fail: warn once and carry on. */
static void save_settings(const Config *cfg, const char *path) {
    if (!config_save(cfg, path))
        fprintf(stderr, "xquest: could not save settings to %s\n", path);
}

static void usage(const char *prog) {
    printf("Usage: %s [options]\n\n"
           "  --play [FILE]     play back a demo (default: xquest.dmo in the\n"
           "                    config dir) and return to the menu\n"
           "  --record [FILE]   record the next game to FILE\n"
           "  --scale N         window size multiplier of 320x240 (%d-%d), or\n"
           "                    'auto' (the default) to fit your screen\n"
           "  --fullscreen      start fullscreen\n"
           "  --windowed        start windowed, whatever was saved last\n"
           "  --dump-frames F   with --play, write raw 320x240 BGRA frames to F\n"
           "                    (or - for stdout) as fast as possible, for\n"
           "                    encoding to video. Pipe into ffmpeg.\n"
           "  --help            show this message\n\n"
           "F11 or Alt+Enter toggles fullscreen at any time, and Ctrl+plus /\n"
           "Ctrl+minus resize the window a step at a time. XQUEST_SCALE and\n"
           "XQUEST_FULLSCREEN set the same things from the environment. The\n"
           "size and fullscreen state you leave are remembered in xquest.win,\n"
           "beside xquest.cfg.\n\n"
           "With no options the game starts normally. A demo file also drives\n"
           "attract mode: the menu plays it after %d seconds idle.\n",
           prog, SCALE_MIN, SCALE_MAX, MENU_IDLE_SECONDS);
}

int main(int argc, char **argv) {
    /* AppImage / portable override: XQUEST_DATA_DIR env var takes precedence. */
    const char *asset_dir = getenv("XQUEST_DATA_DIR");
    if (!asset_dir || asset_dir[0] == '\0') asset_dir = ASSET_DIR;

    const char *play_arg = NULL, *record_arg = NULL, *dump_arg = NULL;
    bool want_play = false, want_record = false;
    /* -1 means "nobody asked": fall back to the saved preference, then to
       fitting the display. */
    int scale      = -1;
    int fullscreen = -1;

    const char *env = getenv("XQUEST_SCALE");
    if (env && env[0] != '\0') {
        scale = parse_scale(env);
        if (scale < 0)
            fprintf(stderr, "xquest: ignoring XQUEST_SCALE=%s (want %d-%d or auto)\n",
                    env, SCALE_MIN, SCALE_MAX);
    }
    env = getenv("XQUEST_FULLSCREEN");
    if (env && env[0] != '\0') fullscreen = (env[0] != '0');
    for (int i = 1; i < argc; i++) {
        /* An optional filename may follow; anything starting with '-' is the
           next option, not a filename. */
        if (strcmp(argv[i], "--play") == 0) {
            want_play = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') play_arg = argv[++i];
        } else if (strcmp(argv[i], "--record") == 0) {
            want_record = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') record_arg = argv[++i];
        } else if (strcmp(argv[i], "--fullscreen") == 0) {
            fullscreen = 1;
        } else if (strcmp(argv[i], "--windowed") == 0) {
            fullscreen = 0;
        } else if (strcmp(argv[i], "--scale") == 0) {
            if (i + 1 >= argc || (scale = parse_scale(argv[i + 1])) < 0) {
                fprintf(stderr, "xquest: --scale needs a number from %d to %d, "
                                "or 'auto'\n", SCALE_MIN, SCALE_MAX);
                return 1;
            }
            i++;
        } else if (strcmp(argv[i], "--dump-frames") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "xquest: --dump-frames needs a file (or -)\n");
                return 1;
            }
            dump_arg = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "xquest: unknown option '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }
    if (want_play && want_record) {
        fprintf(stderr, "xquest: --play and --record are mutually exclusive\n");
        return 1;
    }
    if (dump_arg && !want_play) {
        fprintf(stderr, "xquest: --dump-frames needs --play\n");
        return 1;
    }

    /* Frame dump: the renderer's own 320x240 buffer, so the capture is
       pixel-exact and never touches anything else on screen. */
    FILE *dump = NULL;
    if (dump_arg) {
        dump = (strcmp(dump_arg, "-") == 0) ? stdout : fopen(dump_arg, "wb");
        if (!dump) {
            fprintf(stderr, "xquest: cannot write frames to %s\n", dump_arg);
            return 1;
        }
    }
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    /* Where the window was left last time. Its own file, not xquest.cfg:
       see config.h. Anything asked for on the command line or in the
       environment wins over it. */
    WindowPrefs wprefs;
    char wprefs_path[512];
    window_prefs_path(wprefs_path, sizeof(wprefs_path), asset_dir);
    window_prefs_load(&wprefs, wprefs_path);
    if (scale < 0)      scale = wprefs.scale;   /* 0 there also means auto */
    if (fullscreen < 0) fullscreen = wprefs.fullscreen ? 1 : 0;

    bool auto_scale = (scale == 0);

    /* A comfortable default leaves a 10% margin for panels and the title
       bar; a scale the user asked for still has to fit on the screen. */
    scale = auto_scale ? renderer_fit_scale(0, 90)
                       : (scale > renderer_fit_scale(0, 100)
                             ? renderer_fit_scale(0, 100) : scale);

    SDL_Window *win = SDL_CreateWindow(
        "XQuest",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        320 * scale, 240 * scale,
        SDL_WINDOW_RESIZABLE |
            (fullscreen > 0 ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    Assets a;
    if (assets_load(&a, asset_dir) != 0) {
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    char snd_path[512];
    snprintf(snd_path, sizeof(snd_path), "%s/xquest.snd", asset_dir);
    audio_init(snd_path);   /* non-fatal: game runs silently if audio fails */

    Renderer r;
    if (renderer_init(&r, win, a.palette) != 0) {
        audio_free();
        assets_free(&a);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }
    /* Started fullscreen, the window already reports the screen size, so
       state the windowed scale to drop back to. */
    r.scale = scale;

    /* One line on stderr saying what we picked, as mario-final-sdl does: the
       first thing worth knowing in a "the window is the wrong size" report. */
    fprintf(stderr, "xquest: %dx scale (%dx%d)%s%s\n",
            scale, RENDER_W * scale, RENDER_H * scale,
            auto_scale ? " [auto]" : "",
            renderer_is_fullscreen(&r) ? " [fullscreen]" : "");

    /* gamespeed per difficulty: Wimp→45, Timid→54, Average→64, Tricky→77, Inhuman→96 */
    static const int diff_speed[5] = {45, 54, 64, 77, 96};

    /* Player settings, persisted between runs in the original xquest.cfg
       format. A missing or unreadable file just means factory defaults. */
    Config cfg;
    char   cfg_path[512];
    config_path(cfg_path, sizeof(cfg_path), asset_dir);
    config_load(&cfg, cfg_path);
    int diff = cfg.player[0].difficulty;

    /* Demo paths. The attract-mode demo lives alongside the settings. */
    char demo_path[512];
    user_file_path(demo_path, sizeof(demo_path), asset_dir, "xquest.dmo");
    if (play_arg)   snprintf(demo_path, sizeof(demo_path), "%s", play_arg);
    char record_path[512];
    snprintf(record_path, sizeof(record_path), "%s",
             record_arg ? record_arg : demo_path);

    /* High score table - load once, shared across sessions. Stays its own
       xquest.scr in the original binary format; it just needs to live
       somewhere writable, since an installed asset dir is read-only. */
    HiTable ht;
    char hi_path[512];
    user_file_path(hi_path, sizeof(hi_path), asset_dir, "xquest.scr");
    if (!hi_load(&ht, hi_path)) {
        /* First run against a read-only install: seed from the bundled table
           so the shipped scores survive, then save to the writable copy. */
        char bundled[512];
        snprintf(bundled, sizeof(bundled), "%s/xquest.scr", asset_dir);
        hi_load(&ht, bundled);
        hi_save(&ht, hi_path);
    }

    /* ---- Outer loop: menu → game → game-over → menu ---- */
    /* A demo file present at startup enables attract mode. */
    Demo attract;
    bool have_demo = demo_load(&attract, demo_path);
    if (have_demo) demo_free(&attract);

    bool play_now   = false; /* this pass through the loop is a demo playback */
    bool record_now = want_record;  /* --record starts a game straight away */
    if (want_play) {
        if (!have_demo) {
            fprintf(stderr, "xquest: cannot read demo file %s\n", demo_path);
            renderer_destroy(&r); audio_free(); assets_free(&a);
            SDL_DestroyWindow(win); SDL_Quit();
            return 1;
        }
        play_now = true;
    }

    for (;;) {
        int chosen;
        if (play_now || record_now) {
            chosen = diff;   /* skip the menu straight into the demo */
        } else {
            chosen = run_menu(&a, &r, win, &ht, hi_path, &diff, have_demo);
            if (chosen == MENU_DEMO_TIMEOUT || chosen == MENU_PLAY_DEMO) {
                play_now = true;
                chosen   = diff;
            } else if (chosen == MENU_RECORD_DEMO) {
                record_now = true;
                chosen     = diff;
            }
        }

        /* Save as soon as it changes, so the setting survives a crash or a
           kill rather than only a clean exit. */
        if (diff != cfg.player[0].difficulty) {
            cfg.player[0].difficulty = diff;
            save_settings(&cfg, cfg_path);
        }

        if (chosen < 0) break;   /* user chose Quit or closed window */

        /* Demo state for this game. Playback replays a recording; recording
           captures one. Both are deterministic given the same seed, so a
           demo this port records replays exactly. */
        Demo     demo;
        bool     playing   = false;
        bool     recording = false;
        int      demo_ptr  = 0;
        uint32_t seed      = (uint32_t)SDL_GetTicks();
        int      game_diff = diff;

        if (play_now) {
            if (demo_load(&demo, demo_path)) {
                playing   = true;
                seed      = demo.seed;
                /* Replaying at a different difficulty would change enemy
                   speed and desync immediately. */
                game_diff = demo_difficulty(&demo);
                if (game_diff < 0 || game_diff > 4) game_diff = diff;
            } else {
                fprintf(stderr, "xquest: cannot read demo file %s\n", demo_path);
                play_now = false;
                continue;
            }
        } else if (record_now) {
            recording = true;
        }

        Input     inp;
        GameState gs;
        Starfield sf;
        input_init(&inp);
        game_init(&gs, a.ship[0].w, a.ship[0].h, seed);
        if (recording) demo_start(&demo, seed, &cfg);
        gs.diff_level = game_diff;
        gs.gamespeed  = diff_speed[game_diff];
        level_init(&gs, a.gate_left.w, a.gate_right.w);
        starfield_init(&sf, 0);

        int  running      = 1;
        bool quit_app     = false;   /* window closed: exit, don't bounce to the menu */
        bool game_over    = false;   /* true only when lives reach 0 */
        bool paused       = false;
        bool quit_confirm = false;   /* ESC: showing "QUIT?" prompt */
        uint32_t last_tick = SDL_GetTicks();

        while (running) {
            input_frame_begin(&inp);

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (renderer_display_event(&r, &ev)) continue;
                if (ev.type == SDL_QUIT) { running = 0; quit_app = true; }
                if (ev.type == SDL_KEYDOWN) {
                    SDL_Keycode sym = ev.key.keysym.sym;
                    if (quit_confirm) {
                        if (sym == SDLK_y) {
                            /* Pascal: GameOver=true, Score=0 → no high score */
                            gs.score = 0;
                            game_over = true;
                            running   = 0;
                        } else {
                            quit_confirm = false;   /* any other key: resume */
                        }
                    } else if (sym == SDLK_ESCAPE) {
                        quit_confirm = true;
                    } else if (sym == SDLK_p) {
                        paused = !paused;
                    }
                }
                input_event(&inp, &ev);
            }

            uint32_t now = SDL_GetTicks();
            /* Dumping runs flat out rather than in real time: exactly one
               tick per rendered frame, no waiting on the clock. */
            if (dump) last_tick = now - TICK_MS;
            if (paused || quit_confirm) {
                /* Drain last_tick so resuming doesn't avalanche ticks */
                last_tick = now;
            }
            while (!paused && !quit_confirm && now - last_tick >= TICK_MS) {
                last_tick += TICK_MS;

                int dx = 0, dy = 0;
                if (inp.key[SDL_SCANCODE_LEFT]  || inp.key[SDL_SCANCODE_A]) dx -= KEYBOARD_STEP;
                if (inp.key[SDL_SCANCODE_RIGHT] || inp.key[SDL_SCANCODE_D]) dx += KEYBOARD_STEP;
                if (inp.key[SDL_SCANCODE_UP]    || inp.key[SDL_SCANCODE_W]) dy -= KEYBOARD_STEP;
                if (inp.key[SDL_SCANCODE_DOWN]  || inp.key[SDL_SCANCODE_S]) dy += KEYBOARD_STEP;
                dx += inp.mouse_dx + inp.joy_dx;
                dy += inp.mouse_dy + inp.joy_dy;

                bool brake = inp.key[SDL_SCANCODE_LSHIFT] || inp.key[SDL_SCANCODE_RSHIFT]
                          || inp.key[SDL_SCANCODE_KP_5]
                          || inp.joy_brake;

                bool exploding = (gs.ship_explode_timer > 0);

                bool want_fire = inp.fire_pressed;
                bool want_held = inp.fire_held;
                bool want_bomb = inp.smart_bomb_pressed;

                if (playing) {
                    /* Any fire or bomb press aborts the demo, as in the
                       original (MoveShip sets GameOver on a button click). */
                    if (want_fire || want_bomb) { running = 0; break; }
                    if (demo_ptr >= demo.num_frames) { running = 0; break; }

                    const DemoFrame *fr = &demo.frames[demo_ptr];
                    gs.demo_override = true;
                    gs.demo_delx = fr->delx;
                    gs.demo_dely = fr->dely;
                    want_fire = (fr->but & DEMO_BUT_FIRE)      != 0;
                    want_bomb = (fr->but & DEMO_BUT_BOMB)      != 0;
                    want_held = (fr->but & DEMO_BUT_FIRE_HELD) != 0;
                    dx = dy = 0;
                    brake = false;
                } else {
                    gs.demo_override = false;
                }

                if (!exploding) {
                    uint8_t but = 0;
                    if (want_fire) { shoot(&gs, &a); but |= DEMO_BUT_FIRE; }
                    /* RapidFire: auto-fire every 4th tick while button held */
                    if (gs.powerup_timer[PU_RAPID] > 0 && !want_fire &&
                        want_held && (gs.frame_count & 3) == 0) {
                        shoot(&gs, &a);
                        but |= DEMO_BUT_FIRE_HELD;
                    }
                    if (want_bomb) { fire_smart_bomb(&gs, &a); but |= DEMO_BUT_BOMB; }
                    game_tick(&gs, &a, dx, dy, brake);

                    /* One frame per ticked frame, on both paths, so a
                       recording and its playback stay in step. */
                    if (playing)   demo_ptr++;
                    if (recording) demo_append(&demo, gs.rec_delx, gs.rec_dely, but);

                    level_check_pickups(&gs);
                    powerups_tick(&gs);
                }
                missiles_tick(&gs, &a);
                entities_tick(&gs, &a);
                emissiles_tick(&gs, &a);
                if (!exploding) check_collisions(&gs, &a);
                starfield_step(&sf);

                /* Level exit: ship flew through open gate */
                if (gs.level_complete) {
                    int next_level = gs.level + 1;
                    if (next_level > MAX_LEVELS) { game_over = true; running = 0; break; }
                    long     saved_score = gs.score;
                    int      saved_lives = gs.lives;
                    int      saved_bombs = gs.bombs;
                    uint32_t saved_rng   = gs.rng;
                    game_init(&gs, a.ship[0].w, a.ship[0].h, saved_rng);
                    gs.score = saved_score;
                    gs.level = next_level;
                    gs.lives = saved_lives;
                    gs.bombs = saved_bombs;
                    gs.diff_level = game_diff;
                    gs.gamespeed  = diff_speed[game_diff];
                    level_init(&gs, a.gate_left.w, a.gate_right.w);
                    break;
                }

                /* Start explosion on first hit */
                if (gs.ship_destroyed) {
                    gs.ship_destroyed = false;
                    ship_destroy(&gs, &a);
                }

                /* Count down explosion; respawn or game over when it expires */
                if (gs.ship_explode_timer > 0 && --gs.ship_explode_timer == 0) {
                    gs.lives--;
                    if (gs.lives <= 0) { game_over = true; running = 0; break; }
                    long saved_score = gs.score;
                    int  saved_level = gs.level;
                    int  saved_lives = gs.lives;
                    int  saved_bombs = gs.bombs;
                    game_init(&gs, a.ship[0].w, a.ship[0].h, gs.rng);
                    gs.score = saved_score;
                    gs.level = saved_level;
                    gs.lives = saved_lives;
                    gs.bombs = saved_bombs;
                    gs.diff_level = game_diff;
                    gs.gamespeed  = diff_speed[game_diff];
                    level_init(&gs, a.gate_left.w, a.gate_right.w);
                }
            }

            /* Render */
            render_clear_game(&r, 0);
            render_clear_hud(&r, 1);
            render_starfield(&r, &sf);
            render_world_border(&r, &a, &gs);
            render_enemy_gates(&r, &a, &gs);
            render_objects(&r, &a, &gs);
            render_missiles(&r, &a, &gs);
            render_emissiles(&r, &a, &gs);
            render_enemies(&r, &a, &gs);

            if (gs.ship_explode_timer == 0)
                render_sprite_cam(&r, &a.ship[gs.ship.dir],
                                  gs.ship.x, gs.ship.y,
                                  gs.cam_x,  gs.cam_y);

            if (gs.smart_bomb_flash > 0)
                render_flash_overlay(&r, gs.smart_bomb_flash);
            if (quit_confirm) {
                int tw = comix_text_width(&a, "QUIT?");
                render_comix_text(&r, &a, (320 - tw) / 2, 100, 15, "QUIT?");
            } else if (paused) {
                int tw = comix_text_width(&a, "PAUSE");
                render_comix_text(&r, &a, (320 - tw) / 2, 100, 15, "PAUSE");
            }
            render_hud(&r, &a, &gs);
            renderer_present(&r);
            if (dump)
                fwrite(r.buf, sizeof(uint32_t), RENDER_W * RENDER_H, dump);
        }

        input_shutdown(&inp);

        if (quit_app) {
            /* Still save a recording cut short by closing the window. */
            if (recording && demo.num_frames > 0)
                demo_save(&demo, record_path);
            if (playing || recording) demo_free(&demo);
            break;
        }

        if (recording) {
            record_now = false;   /* one recording, then back to the menu */
            if (demo_save(&demo, record_path)) {
                printf("xquest: recorded %d frames to %s\n",
                       demo.num_frames, record_path);
                /* A freshly recorded demo enables attract mode straight away. */
                if (strcmp(record_path, demo_path) == 0) have_demo = true;
            } else {
                fprintf(stderr, "xquest: could not write demo to %s\n", record_path);
            }
        }
        if (playing || recording) demo_free(&demo);

        if (playing) {
            /* A demo is not a game: no score, no hall of fame. */
            playing  = false;
            play_now = false;
            if (want_play) break;   /* --play: one playback, then exit */
            continue;
        }

        if (game_over)
            run_game_over(&a, &r, &ht, hi_path, gs.diff_level, gs.score, gs.level);
        /* If not game_over (player pressed Escape), loop back to menu silently */
    }

    /* The original wrote xquest.cfg unconditionally on exit (WriteDefaults at
       the end of xquest.pas), which is what creates the file on a first run. */
    save_settings(&cfg, cfg_path);

    /* Remember the window for next time. Like the settings above, a failure
       here is a shrug, not an error: the game still ran. */
    /* An auto-fitted run nobody resized stays auto, so the window keeps
       fitting itself if the screen or the monitor changes. Touch the size at
       all, by hotkey or by dragging, and that exact size is what we keep. */
    wprefs.scale      = (auto_scale && r.scale == scale) ? 0 : r.scale;
    wprefs.fullscreen = renderer_is_fullscreen(&r);
    if (!window_prefs_save(&wprefs, wprefs_path))
        fprintf(stderr, "xquest: could not save window size to %s\n", wprefs_path);

    if (dump && dump != stdout) fclose(dump);

    renderer_destroy(&r);
    audio_free();
    assets_free(&a);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
