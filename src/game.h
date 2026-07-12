#ifndef GAMEH
#define GAMEH

#include <stdint.h>
#include "bitboard.h"

#define BOARD_SIZE 64


typedef struct {
    uint8_t player_w;
    uint8_t player_b;

    uint8_t board[BOARD_SIZE];

    /* Bitboard representation, kept in sync with the mailbox 'board' above and
     * used as the source of truth for move generation. pieces is indexed
     * [color][piece type], where color is BB_WHITE/BB_BLACK and piece type is
     * the PAWN..KING enum value (index 0 is unused). occ holds per-color
     * occupancy; occ_all is both colors combined.
     */
    Bitboard pieces[2][7];
    Bitboard occ[2];
    Bitboard occ_all;

    // game info bit map
    // bit 0 - move (0 = white, 1 = black)
    // bit 1 - castling rights (white, kingside) (starts as 1)
    // bit 2 - castling rights (white, queenside) (starts as 1)
    // bit 3 - castling rights (black, kingside) (starts as 1)
    // bit 4 - castling rights (black, queenside) (starts as 1)
    // bit 5 - none
    // bit 6 - none
    // bit 7 - none
    // uint8_t game_info;
    uint8_t move : 1;
    uint8_t castle_kingside_w : 1;
    uint8_t castle_queenside_w : 1;
    uint8_t castle_kingside_b : 1;
    uint8_t castle_queenside_b : 1;
    uint8_t none : 3;					// extra 3 bit padding to make a full byte	

    uint8_t w_en_passant_square;		// en passant attack square for white (changed on black move)
                                        // if larger than 63, invalid, if <= 63, valid
    uint8_t b_en_passant_square;		// en passant attack square for black (changed on white move)
                                        // if larger than 63, invalid, if <= 63, valid

    uint8_t king_square_w;
    uint8_t king_square_b;
} Game;


Game *create_game();
Game *clone_game(Game *game);
void delete_game(Game *game);
void change_turn(Game *game);
void set_player_w(Game *game, uint8_t player_w);
void set_player_b(Game *game, uint8_t player_b);
void print_all_square_values(Game *game);
void set_default_board(Game *game);
void rebuild_bitboards(Game *game);
void move_piece(Game *game, uint32_t move);
void modify_castling_rights(Game *game, uint32_t move);
void print_board(Game *game);
void print_board_reverse(Game *game);

Game *load_fen_game(const char *filepath);
Game *parse_fen(const char *fen);
void game_to_fen(Game *game, char *buffer);

#endif
