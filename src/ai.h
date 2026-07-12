#ifndef AIH
#define AIH

#include <stdint.h>
#include "game.h"
#include "movelist.h"

uint32_t get_computer_move(Game *game, uint8_t player);
void print_possible_moves(Game *game);
uint32_t perft(Game *game, int8_t depth);
void run_perft_test(Game *game, uint32_t max_depth);
int run_validation();
void run_benchmark();
int run_crosscheck(Game *game, int depth);

#endif
