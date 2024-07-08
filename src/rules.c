#include <stdint.h>
#include "rules.h"
#include "game.h"
#include "player.h"
#include "piece.h"

uint8_t is_legal_move(Game *game, uint16_t move) {
    // get the moving player
    uint8_t active_player = game->move ? PLAYERW : PLAYERB;

    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    // check if piece is on from square
    uint8_t piece = game->board[from];
    if (!is_piece(piece)) {
        return 0;
    }

    // check if piece is active player's piece
    if (active_player == PLAYERW) {
        if (!is_white(piece)) {
            return 0;
        }
    }
    else {
        if (is_white(piece)) {
            return 0;
        }
    }

    // check if move is valid for that piece
    if (!is_valid_piece_move(game, move)) {
        return 0;
    }

    // check if move goes through pieces or lands on own piece (not allowed!)
    if (is_move_obstructed(game, move)) {
        return 0;
    }

    // check if move is puts self in check (which would be illegal)
    Game *clone = clone_game(game);
    move_piece(clone, move);
    if (is_in_check(clone, active_player)) {
        delete_game(clone);
        return 0;
    }
    delete_game(clone);


    return 1;
}


uint8_t is_in_check(Game *game, uint8_t player) {
    if (player == PLAYERW) {

    }
    else {

    }

    return 0;
}


uint8_t is_valid_piece_move(Game *game, uint16_t move) {
    return 1;
}


uint8_t is_move_obstructed(Game *game, uint16_t move) {
    return 1;
}
