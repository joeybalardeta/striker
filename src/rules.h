#ifndef RULESH
#define RULESH

#include <stdint.h>
#include "game.h"
#include "movelist.h"

// defines
#define CHECKMATE 1
#define STALEMATE 2
#define DRAW_IM 3
#define DRAW_R 4

// major move validator functions
uint8_t is_legal_move(Game *game, uint32_t move);
uint8_t is_in_check(Game *game, uint8_t player);
uint8_t is_checkmate(Game *game);
uint8_t is_draw(Game *game);
uint8_t is_valid_piece_move(Game *game, uint32_t move);
uint8_t is_move_unobstructed(Game *game, uint32_t move);
uint8_t is_piece_attacking(Game *game, uint8_t attacker, uint8_t target);
uint8_t is_attacking(Game *game, uint8_t attacking_player, uint8_t target);
uint8_t get_attacking_square(Game *game, uint8_t attacking_player, uint8_t target);

// piece move validator functions
uint8_t is_valid_pawn_move(Game *game, uint32_t move);
uint8_t is_valid_knight_move(Game *game, uint32_t move);
uint8_t is_valid_bishop_move(Game *game, uint32_t move);
uint8_t is_valid_rook_move(Game *game, uint32_t move);
uint8_t is_valid_queen_move(Game *game, uint32_t move);
uint8_t is_valid_king_move(Game *game, uint32_t move);

// extra 
uint32_t add_move_flags(Game *game, uint32_t move);
uint8_t get_king_square(Game *game, uint8_t player);
uint8_t can_castle(Game *game, uint8_t player, uint8_t queenside);
uint8_t is_pawn_promotion_move(Game *game, uint32_t move);
MoveList *get_possible_moves(Game *game);

#endif
