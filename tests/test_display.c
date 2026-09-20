/* Display hotkeys: F11 and Alt+Enter toggle fullscreen, Ctrl+plus/minus
   resize the window, and the game's own keys pass through untouched. Runs
   against SDL's dummy video driver, so it needs no screen. */
#include "render.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int failures = 0;

#define CHECK(cond, ...) do {                                   \
    if (!(cond)) { printf("FAIL %s:%d: ", __FILE__, __LINE__);  \
                   printf(__VA_ARGS__); printf("\n");           \
                   failures++; }                                \
} while (0)

static SDL_Event key(Uint32 type, SDL_Keycode sym, Uint16 mod, Uint8 repeat) {
    SDL_Event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type            = type;
    ev.key.repeat      = repeat;
    ev.key.keysym.sym  = sym;
    ev.key.keysym.mod  = mod;
    return ev;
}

static bool is_full(SDL_Window *w) {
    return (SDL_GetWindowFlags(w) & SDL_WINDOW_FULLSCREEN_DESKTOP) != 0;
}

int main(void) {
    /* setenv, not SDL_SetHint: SDL_HINT_VIDEODRIVER only arrived in 2.0.22,
       and this has to work on whatever CI is running. */
    SDL_setenv("SDL_VIDEODRIVER", "dummy", 1);
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SKIP: no SDL video: %s\n", SDL_GetError());
        return 0;
    }
    printf("video driver: %s\n", SDL_GetCurrentVideoDriver());

    SDL_Window *win = SDL_CreateWindow("t", 0, 0, 320, 240, SDL_WINDOW_HIDDEN);
    if (!win) {
        printf("SKIP: no window: %s\n", SDL_GetError());
        SDL_Quit();
        return 0;
    }

    /* Only the window and scale fields are touched, so no SDL_Renderer is
       needed here. */
    Renderer r;
    memset(&r, 0, sizeof(r));
    r.window = win;
    r.scale  = 2;

    CHECK(renderer_fit_scale(0, 90) >= SCALE_MIN &&
          renderer_fit_scale(0, 90) <= SCALE_MAX, "fit scale out of range");
    CHECK(renderer_fit_scale(0, 90) <= renderer_fit_scale(0, 100),
          "90%% fit is larger than the 100%% ceiling");

    SDL_Event ev;

    ev = key(SDL_KEYDOWN, SDLK_F11, KMOD_NONE, 0);
    CHECK(renderer_display_event(&r, &ev), "F11 not consumed");
    CHECK(is_full(win), "F11 did not go fullscreen");

    ev = key(SDL_KEYDOWN, SDLK_F11, KMOD_NONE, 0);
    renderer_display_event(&r, &ev);
    CHECK(!is_full(win), "second F11 did not return to a window");

    /* Auto-repeat must not flap the window mode. */
    ev = key(SDL_KEYDOWN, SDLK_F11, KMOD_NONE, 1);
    CHECK(!renderer_display_event(&r, &ev), "repeat consumed");
    CHECK(!is_full(win), "repeat toggled fullscreen");

    ev = key(SDL_KEYDOWN, SDLK_RETURN, KMOD_LALT, 0);
    CHECK(renderer_display_event(&r, &ev), "Alt+Enter not consumed");
    CHECK(is_full(win), "Alt+Enter did not go fullscreen");

    /* The key-up half is swallowed too, so a loop watching the fire button
       never sees a stray Return release. */
    ev = key(SDL_KEYUP, SDLK_RETURN, KMOD_LALT, 0);
    CHECK(renderer_display_event(&r, &ev), "Alt+Enter key-up not consumed");

    ev = key(SDL_KEYDOWN, SDLK_RETURN, KMOD_LALT, 0);
    renderer_display_event(&r, &ev);
    CHECK(!is_full(win), "second Alt+Enter did not return to a window");

    /* Plain Return is the fire button: it must pass straight through. */
    ev = key(SDL_KEYDOWN, SDLK_RETURN, KMOD_NONE, 0);
    CHECK(!renderer_display_event(&r, &ev), "plain Return consumed");
    CHECK(!is_full(win), "plain Return toggled fullscreen");

    ev = key(SDL_KEYDOWN, SDLK_ESCAPE, KMOD_NONE, 0);
    CHECK(!renderer_display_event(&r, &ev), "Escape consumed");

    /* Ctrl+plus / Ctrl+minus step the window size. The ceiling is whatever
       fits the display, so only ask for a step we know is allowed. */
    int ceiling = renderer_fit_scale(0, 100);
    r.scale = 2;
    ev = key(SDL_KEYDOWN, SDLK_MINUS, KMOD_LCTRL, 0);
    CHECK(renderer_display_event(&r, &ev), "Ctrl+minus not consumed");
    CHECK(r.scale == 1, "Ctrl+minus gave scale %d, want 1", r.scale);

    ev = key(SDL_KEYDOWN, SDLK_MINUS, KMOD_LCTRL, 0);
    renderer_display_event(&r, &ev);
    CHECK(r.scale == 1, "Ctrl+minus went below 1x (scale %d)", r.scale);

    if (ceiling >= 2) {
        ev = key(SDL_KEYDOWN, SDLK_EQUALS, KMOD_LCTRL, 0);
        CHECK(renderer_display_event(&r, &ev), "Ctrl+plus not consumed");
        CHECK(r.scale == 2, "Ctrl+plus gave scale %d, want 2", r.scale);
    }

    r.scale = ceiling;
    ev = key(SDL_KEYDOWN, SDLK_KP_PLUS, KMOD_LCTRL, 0);
    renderer_display_event(&r, &ev);
    CHECK(r.scale == ceiling, "grew past what fits the display (scale %d, ceiling %d)",
          r.scale, ceiling);

    /* Plus without Ctrl is not ours. */
    r.scale = 2;
    ev = key(SDL_KEYDOWN, SDLK_EQUALS, KMOD_NONE, 0);
    CHECK(!renderer_display_event(&r, &ev), "plain plus consumed");
    CHECK(r.scale == 2, "plain plus resized the window");

    SDL_DestroyWindow(win);
    SDL_Quit();

    printf(failures ? "test_display: %d failure(s)\n" : "test_display: ok\n",
           failures);
    return failures ? 1 : 0;
}
