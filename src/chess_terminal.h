#ifndef CHESSTERMINALH
#define CHESSTERMINALH

#include <stdint.h>
#include "game.h"

#define EXIT_INPUT 9

void chess_terminal();
void execute_option(uint32_t option);
void print_options();

void game_loop(Game *game);
uint8_t game_tick(Game *game);

#endif
