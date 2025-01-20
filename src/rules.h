#ifndef RULESH
#define RULESH

#include <stdint.h>
#include "game.h"
#include "movelist.h"

uint8_t is_legal_move(Game *game, uint32_t move);

uint8_t is_in_check(Game *game, uint8_t player);

uint8_t is_valid_piece_move(Game *game, uint32_t move);

uint8_t is_move_unobstructed(Game *game, uint32_t move);

uint8_t is_attacking(Game *game, uint8_t target, uint8_t attacker);

// piece move validator functions
uint8_t is_valid_pawn_move(Game *game, uint32_t move);
uint8_t is_valid_knight_move(Game *game, uint32_t move);
uint8_t is_valid_bishop_move(Game *game, uint32_t move);
uint8_t is_valid_rook_move(Game *game, uint32_t move);
uint8_t is_valid_queen_move(Game *game, uint32_t move);
uint8_t is_valid_king_move(Game *game, uint32_t move);

MoveList *get_possible_moves(Game *game, uint8_t player);

#endif
