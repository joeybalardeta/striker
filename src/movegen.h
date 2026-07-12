#ifndef MOVEGENH
#define MOVEGENH

#include <stdint.h>
#include "game.h"
#include "movelist.h"

/* game-state result codes (used by the interactive terminal) */
#define CHECKMATE 1
#define STALEMATE 2
#define DRAW_IM 3
#define DRAW_R 4

/* Fills 'out' with every legal move for the side to move, encoded in the
 * uint32_t move format from move.h (from | to<<8 | flags). This is the bitboard
 * generator that replaces the old mailbox rules.c.
 */
void generate_moves(Game *game, MoveList *out);

/* Convenience wrapper that allocates a MoveList and fills it via generate_moves. */
MoveList *get_possible_moves(Game *game);

/* Returns non-zero if square 'sq' is attacked by any piece of color 'by'
 * (BB_WHITE or BB_BLACK), using the current board occupancy.
 */
uint8_t square_attacked(Game *game, uint8_t sq, int by);

/* Returns non-zero if 'player' (PLAYERW or PLAYERB) has their king in check. */
uint8_t is_in_check(Game *game, uint8_t player);

/* Returns CHECKMATE if the side to move is checkmated, else 0. */
uint8_t is_checkmate(Game *game);

/* Returns STALEMATE / DRAW_IM if the side to move is stalemated or the position
 * is a draw by insufficient material, else 0.
 */
uint8_t is_draw(Game *game);

#endif
