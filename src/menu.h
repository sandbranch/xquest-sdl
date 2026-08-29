#pragma once
#include <SDL2/SDL.h>
#include "assets.h"
#include "render.h"
#include "hiscore.h"

/* run_menu returns this when the menu sat idle long enough to start the
   attract-mode demo, matching MenuTimeOut in the original. */
#define MENU_DEMO_TIMEOUT -2

/* Chosen from the Demo entries, mirroring DemoMenu in the original. */
#define MENU_PLAY_DEMO    -3
#define MENU_RECORD_DEMO  -4

/* Seconds of inactivity before that happens. */
#define MENU_IDLE_SECONDS 30

/* Run the main menu. Blocks until the user makes a choice.
   Returns the chosen difficulty level (0=Wimp .. 4=Inhuman), -1 if the user
   quit, or MENU_DEMO_TIMEOUT if it idled and the caller should play a demo.
   ht/hi_path are used for the Hall of Fame menu item.

   demo_available: pass false when there is no demo file, and the menu will
   simply keep waiting instead of timing out.

   *diff is in/out: it seeds the highlighted difficulty (from the saved
   config) and is updated as the player cycles it, so a change sticks even
   when they then quit rather than starting a game. */
int run_menu(const Assets *a, Renderer *r, SDL_Window *win,
             HiTable *ht, const char *hi_path, int *diff,
             bool demo_available);

/* Show game-over box, check/insert high score, display hall of fame.
   diff=0-4, score/level from the completed game. Loads from ht in place. */
void run_game_over(const Assets *a, Renderer *r,
                   HiTable *ht, const char *hi_path,
                   int diff, long score, int level);

/* Display the hall of fame for diff (0-4) over the starfield.
   Returns when user presses a key or seconds elapses. */
void run_halloffame(const Assets *a, Renderer *r,
                    const HiTable *ht, int diff, int seconds);
