#pragma once
#include <SDL2/SDL.h>
#include <stdbool.h>

#define JOY_DEAD_ZONE  8000   /* ~25% of 32767 */
#define JOY_MAX_DELTA    10   /* full-deflection delta, matches KEYBOARD_STEP */

typedef struct {
    bool key[SDL_NUM_SCANCODES];
    int  mouse_dx, mouse_dy;     /* relative motion accumulated this frame */
    bool fire_pressed;           /* edge: went down this frame */
    bool fire_held;              /* level: currently held */
    bool smart_bomb;
    /* Gamepad / joystick */
    SDL_GameController *gc;      /* NULL if no recognised gamepad connected */
    SDL_Joystick       *joy;     /* fallback for unrecognised controllers */
    int  joy_axis_x;             /* raw axis value, updated by events */
    int  joy_axis_y;
    int  joy_dx;                 /* scaled analog delta, computed each frame */
    int  joy_dy;
    bool joy_brake;              /* left shoulder / X button */
} Input;

void input_init(Input *inp);
void input_shutdown(Input *inp);  /* close controller handles */

/* Call once at the start of each frame, before processing events. */
void input_frame_begin(Input *inp);

/* Feed one SDL event into the input state. */
void input_event(Input *inp, const SDL_Event *ev);
