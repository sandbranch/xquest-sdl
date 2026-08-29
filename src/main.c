#include <SDL2/SDL.h>
#include <stdio.h>
#include "assets.h"
#include "render.h"
#include "input.h"
#include "game.h"
#include "menu.h"
#include "hiscore.h"
#include "starfield.h"
#include "audio.h"
#include "config.h"

#ifndef ASSET_DIR
#define ASSET_DIR "../xquest"
#endif

#define TICK_MS 15   /* ~67 fps fixed timestep */

/* Settings are a convenience, never a reason to fail: warn once and carry on. */
static void save_settings(const Config *cfg, const char *path) {
    if (!config_save(cfg, path))
        fprintf(stderr, "xquest: could not save settings to %s\n", path);
}

int main(void) {
    /* AppImage / portable override: XQUEST_DATA_DIR env var takes precedence. */
    const char *asset_dir = getenv("XQUEST_DATA_DIR");
    if (!asset_dir || asset_dir[0] == '\0') asset_dir = ASSET_DIR;
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
    for (;;) {
        int chosen = run_menu(&a, &r, win, &ht, hi_path, &diff);

        /* Save as soon as it changes, so the setting survives a crash or a
           kill rather than only a clean exit. */
        if (diff != cfg.player[0].difficulty) {
            cfg.player[0].difficulty = diff;
            save_settings(&cfg, cfg_path);
        }

        if (chosen < 0) break;   /* user chose Quit or closed window */

        Input     inp;
        GameState gs;
        Starfield sf;
        input_init(&inp);
        game_init(&gs, a.ship[0].w, a.ship[0].h, (uint32_t)SDL_GetTicks());
        gs.diff_level = diff;
        gs.gamespeed  = diff_speed[diff];
        level_init(&gs, a.gate_left.w, a.gate_right.w);
        starfield_init(&sf, 0);

        int  running      = 1;
        bool game_over    = false;   /* true only when lives reach 0 */
        bool paused       = false;
        bool quit_confirm = false;   /* ESC: showing "QUIT?" prompt */
        uint32_t last_tick = SDL_GetTicks();

        while (running) {
            input_frame_begin(&inp);

            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type == SDL_QUIT) { running = 0; }
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

                if (!exploding) {
                    if (inp.fire_pressed) shoot(&gs, &a);
                    /* RapidFire: auto-fire every 4th tick while button held */
                    if (gs.powerup_timer[PU_RAPID] > 0 && !inp.fire_pressed &&
                        inp.fire_held && (gs.frame_count & 3) == 0)
                        shoot(&gs, &a);
                    if (bomb_pressed) fire_smart_bomb(&gs, &a);
                    game_tick(&gs, dx, dy, brake);
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
                    gs.diff_level = diff;
                    gs.gamespeed  = diff_speed[diff];
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
                    gs.diff_level = diff;
                    gs.gamespeed  = diff_speed[diff];
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
