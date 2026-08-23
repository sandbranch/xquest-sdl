#include "input.h"
#include <string.h>

/* Scale a raw axis value to a delta in [-JOY_MAX_DELTA, JOY_MAX_DELTA].
   Returns 0 inside the dead zone. */
static int axis_to_delta(int axis) {
    if (axis >  JOY_DEAD_ZONE)
        return ((axis - JOY_DEAD_ZONE) * JOY_MAX_DELTA) / (32767 - JOY_DEAD_ZONE);
    if (axis < -JOY_DEAD_ZONE)
        return ((axis + JOY_DEAD_ZONE) * JOY_MAX_DELTA) / (32767 - JOY_DEAD_ZONE);
    return 0;
}

/* Open the first available game controller.  If none has an SDL mapping,
   fall back to the first raw joystick. */
static void try_open_controller(Input *inp) {
    int n = SDL_NumJoysticks();
    for (int i = 0; i < n; i++) {
        if (SDL_IsGameController(i)) {
            inp->gc = SDL_GameControllerOpen(i);
            if (inp->gc) return;
        }
    }
    /* Fallback: open as raw joystick */
    for (int i = 0; i < n; i++) {
        inp->joy = SDL_JoystickOpen(i);
        if (inp->joy) return;
    }
}

void input_init(Input *inp) {
    memset(inp, 0, sizeof(*inp));
    SDL_SetRelativeMouseMode(SDL_TRUE);
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    try_open_controller(inp);
}

void input_shutdown(Input *inp) {
    if (inp->gc)  { SDL_GameControllerClose(inp->gc);  inp->gc  = NULL; }
    if (inp->joy) { SDL_JoystickClose(inp->joy);        inp->joy = NULL; }
}

void input_frame_begin(Input *inp) {
    inp->mouse_dx    = 0;
    inp->mouse_dy    = 0;
    inp->fire_pressed = false;
    inp->joy_dx = axis_to_delta(inp->joy_axis_x);
    inp->joy_dy = axis_to_delta(inp->joy_axis_y);
}

/* Return the SDL instance ID for whichever device we have open, or -1. */
static SDL_JoystickID our_instance(const Input *inp) {
    if (inp->gc)
        return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(inp->gc));
    if (inp->joy)
        return SDL_JoystickInstanceID(inp->joy);
    return -1;
}

void input_event(Input *inp, const SDL_Event *ev) {
    switch (ev->type) {

    /* ---- Keyboard ---- */
    case SDL_KEYDOWN:
        inp->key[ev->key.keysym.scancode] = true;
        if (ev->key.keysym.scancode == SDL_SCANCODE_SPACE ||
            ev->key.keysym.scancode == SDL_SCANCODE_RETURN) {
            inp->fire_pressed = true;
            inp->fire_held    = true;
        }
        break;
    case SDL_KEYUP:
        inp->key[ev->key.keysym.scancode] = false;
        if (ev->key.keysym.scancode == SDL_SCANCODE_SPACE ||
            ev->key.keysym.scancode == SDL_SCANCODE_RETURN)
            inp->fire_held = false;
        break;

    /* ---- Mouse ---- */
    case SDL_MOUSEMOTION:
        inp->mouse_dx += ev->motion.xrel;
        inp->mouse_dy += ev->motion.yrel;
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (ev->button.button == SDL_BUTTON_LEFT) {
            inp->fire_pressed = true;
            inp->fire_held    = true;
        }
        if (ev->button.button == SDL_BUTTON_RIGHT) inp->smart_bomb = true;
        break;
    case SDL_MOUSEBUTTONUP:
        if (ev->button.button == SDL_BUTTON_LEFT)  inp->fire_held  = false;
        if (ev->button.button == SDL_BUTTON_RIGHT) inp->smart_bomb = false;
        break;

    /* ---- Gamepad device lifecycle ---- */
    case SDL_CONTROLLERDEVICEADDED:
        if (!inp->gc && !inp->joy)
            inp->gc = SDL_GameControllerOpen(ev->cdevice.which);
        break;
    case SDL_CONTROLLERDEVICEREMOVED:
        if (inp->gc &&
            SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(inp->gc))
                == ev->cdevice.which) {
            SDL_GameControllerClose(inp->gc);
            inp->gc = NULL;
            inp->joy_axis_x = inp->joy_axis_y = 0;
            inp->joy_brake = false;
            try_open_controller(inp);
        }
        break;
    case SDL_JOYDEVICEADDED:
        if (!inp->gc && !inp->joy)
            inp->joy = SDL_JoystickOpen(ev->jdevice.which);
        break;
    case SDL_JOYDEVICEREMOVED:
        if (inp->joy && SDL_JoystickInstanceID(inp->joy) == ev->jdevice.which) {
            SDL_JoystickClose(inp->joy);
            inp->joy = NULL;
            inp->joy_axis_x = inp->joy_axis_y = 0;
            inp->joy_brake = false;
        }
        break;

    /* ---- Gamepad axis (left stick → movement) ---- */
    case SDL_CONTROLLERAXISMOTION:
        if (ev->caxis.which != our_instance(inp)) break;
        if (ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX)
            inp->joy_axis_x = ev->caxis.value;
        else if (ev->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY)
            inp->joy_axis_y = ev->caxis.value;
        break;

    /* ---- Gamepad buttons ---- */
    case SDL_CONTROLLERBUTTONDOWN:
        if (ev->cbutton.which != our_instance(inp)) break;
        switch (ev->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
            inp->fire_pressed = true;
            inp->fire_held    = true;
            break;
        case SDL_CONTROLLER_BUTTON_B:
            inp->smart_bomb = true;
            break;
        case SDL_CONTROLLER_BUTTON_X:
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            inp->joy_brake = true;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  inp->joy_axis_x = -32767; break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: inp->joy_axis_x =  32767; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:    inp->joy_axis_y = -32767; break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  inp->joy_axis_y =  32767; break;
        }
        break;
    case SDL_CONTROLLERBUTTONUP:
        if (ev->cbutton.which != our_instance(inp)) break;
        switch (ev->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_A:
            inp->fire_held = false;
            break;
        case SDL_CONTROLLER_BUTTON_B:
            inp->smart_bomb = false;
            break;
        case SDL_CONTROLLER_BUTTON_X:
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            inp->joy_brake = false;
            break;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: inp->joy_axis_x = 0; break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP:
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  inp->joy_axis_y = 0; break;
        }
        break;

    /* ---- Raw joystick fallback ---- */
    case SDL_JOYAXISMOTION:
        if (!inp->joy || ev->jaxis.which != our_instance(inp)) break;
        if (ev->jaxis.axis == 0) inp->joy_axis_x = ev->jaxis.value;
        if (ev->jaxis.axis == 1) inp->joy_axis_y = ev->jaxis.value;
        break;
    case SDL_JOYBUTTONDOWN:
        if (!inp->joy || ev->jbutton.which != our_instance(inp)) break;
        if (ev->jbutton.button == 0) { inp->fire_pressed = true; inp->fire_held = true; }
        if (ev->jbutton.button == 1) inp->smart_bomb = true;
        if (ev->jbutton.button == 2) inp->joy_brake  = true;
        break;
    case SDL_JOYBUTTONUP:
        if (!inp->joy || ev->jbutton.which != our_instance(inp)) break;
        if (ev->jbutton.button == 0) inp->fire_held  = false;
        if (ev->jbutton.button == 1) inp->smart_bomb = false;
        if (ev->jbutton.button == 2) inp->joy_brake  = false;
        break;
    case SDL_JOYHATMOTION:
        if (!inp->joy || ev->jhat.which != our_instance(inp)) break;
        inp->joy_axis_x = (ev->jhat.value & SDL_HAT_RIGHT) ?  32767 :
                          (ev->jhat.value & SDL_HAT_LEFT)  ? -32767 : 0;
        inp->joy_axis_y = (ev->jhat.value & SDL_HAT_DOWN)  ?  32767 :
                          (ev->jhat.value & SDL_HAT_UP)    ? -32767 : 0;
        break;

    default:
        break;
    }
}
