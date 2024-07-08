#ifndef RULESH
#define RULESH

#include <stdint.h>
#include "game.h"

uint8_t is_legal_move(Game *game, uint16_t move);

uint8_t is_in_check(Game *game, uint8_t player);

uint8_t is_valid_piece_move(Game *game, uint16_t move);

uint8_t is_move_obstructed(Game *game, uint16_t move);

#endif
