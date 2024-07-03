#ifndef GAMEH
#define GAMEH

#include <stdint.h>

#define BOARD_SIZE 64


typedef struct {
    uint8_t player_w;
    uint8_t player_b;

    uint8_t board[BOARD_SIZE];

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

    uint8_t en_passant_square;			// if larger than 63, invalid, if <= 63, then valid
} Game;


Game *create_game();
Game *clone_game(Game *game);
void change_turn(Game *game);
void set_player_w(Game *game, uint8_t player_w);
void set_player_b(Game *game, uint8_t player_b);
void set_default_board(Game *game);
void move(Game *game, uint8_t from, uint8_t to);


#endif
