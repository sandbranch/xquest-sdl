#include <SDL2/SDL.h>
#include <stdio.h>
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
           "  --help            show this message\n\n"
           "With no options the game starts normally. A demo file also drives\n"
           "attract mode: the menu plays it after %d seconds idle.\n",
           prog, MENU_IDLE_SECONDS);
}

int main(int argc, char **argv) {
    /* AppImage / portable override: XQUEST_DATA_DIR env var takes precedence. */
    const char *asset_dir = getenv("XQUEST_DATA_DIR");
    if (!asset_dir || asset_dir[0] == '\0') asset_dir = ASSET_DIR;

    const char *play_arg = NULL, *record_arg = NULL;
    bool want_play = false, want_record = false;
    for (int i = 1; i < argc; i++) {
        /* An optional filename may follow; anything starting with '-' is the
           next option, not a filename. */
        if (strcmp(argv[i], "--play") == 0) {
            want_play = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') play_arg = argv[++i];
        } else if (strcmp(argv[i], "--record") == 0) {
            want_record = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') record_arg = argv[++i];
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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow(
        "XQuest",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        960, 720,
        SDL_WINDOW_RESIZABLE);
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
            if (chosen == MENU_DEMO_TIMEOUT) { play_now = true; chosen = diff; }
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

                bool bomb_pressed = inp.smart_bomb
                                 || inp.key[SDL_SCANCODE_SPACE];

                bool want_fire = inp.fire_pressed;
                bool want_held = inp.fire_held;
                bool want_bomb = bomb_pressed;

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
                    game_tick(&gs, dx, dy, brake);

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

    renderer_destroy(&r);
    audio_free();
    assets_free(&a);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
