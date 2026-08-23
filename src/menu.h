#pragma once
#include <SDL2/SDL.h>
#include "assets.h"
#include "render.h"
#include "hiscore.h"

/* Run the main menu. Blocks until the user makes a choice.
   Returns the chosen difficulty level (0=Wimp .. 4=Inhuman), or -1 if the
   user quit. ht/hi_path are used for the Hall of Fame menu item. */
int run_menu(const Assets *a, Renderer *r, SDL_Window *win,
             HiTable *ht, const char *hi_path);

/* Show game-over box, check/insert high score, display hall of fame.
   diff=0-4, score/level from the completed game. Loads from ht in place. */
void run_game_over(const Assets *a, Renderer *r,
                   HiTable *ht, const char *hi_path,
                   int diff, long score, int level);

/* Display the hall of fame for diff (0-4) over the starfield.
   Returns when user presses a key or seconds elapses. */
void run_halloffame(const Assets *a, Renderer *r,
                    const HiTable *ht, int diff, int seconds);
