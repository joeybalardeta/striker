#ifndef AIH
#define AIH

#include <stdint.h>
#include "game.h"
#include "movelist.h"

uint32_t get_computer_move(Game *game, uint8_t player);
void print_possible_moves(Game *game);

#endif
