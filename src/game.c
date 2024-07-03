#include <stdlib.h>
#include <stdint.h>
#include "game.h"
#include "player.h"
#include "piece.h"

Game *create_game() {
    Game *game = malloc(sizeof(Game));
    game->player_w = USER;
    game->player_b = USER;
    
    game->move = 0;
    game->castle_kingside_w = 1;
    game->castle_queenside_w = 1;
    game->castle_kingside_b = 1;
    game->castle_queenside_b = 1;

    return game;
}


Game *clone_game(Game *game) {
    Game *clone = malloc(sizeof(Game));
    clone->player_w = game->player_w;
    clone->player_b = game->player_b;
    clone->move = game->move;
    clone->castle_kingside_w = game->castle_kingside_w;
    clone->castle_queenside_w = game->castle_queenside_w;
    clone->castle_kingside_b = game->castle_kingside_b;
    clone->castle_queenside_b = game->castle_queenside_b;

    return clone;
}	

void change_turn(Game *game) {
    game->move = ~(game->move);
}


void set_player_w(Game *game, uint8_t player_w) {
    game->player_w = player_w;
}


void set_player_b(Game *game, uint8_t player_b) {
    game->player_b = player_b;
}


void set_default_board(Game *game) {
    // pawns
    for (uint8_t i = 0; i < 8; i++) {
        game->board[i + 8] = WHITE | PAWN;
        game->board[i + 48] = BLACK | PAWN;
    }

}


void move(Game *game, uint8_t from, uint8_t to) {
    game->board[to] = game->board[from];
    game->board[from] = 0x00;
}

