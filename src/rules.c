#include <stdio.h>
#include <stdint.h>
#include "rules.h"
#include "game.h"
#include "player.h"
#include "piece.h"

uint8_t is_legal_move(Game *game, uint32_t move) {
    // get the moving player
    uint8_t active_player = !game->move ? PLAYERW : PLAYERB;

    // deserialize move
    uint8_t from = move & 0xFF;
    // uint8_t to = (move >> 8) & 0xFF;

    // check if piece is on from square
    uint8_t piece = game->board[from];
    if (!is_piece(piece)) {
        printf("Move invalidated - no piece on from square!\n");
        return 0;
    }

    // verify piece is active player's piece
    if (active_player == PLAYERW) {
        if (!is_white(piece)) {
            printf("Move invalidated - not a white piece!\n");
            return 0;
        }
    }
    else {
        if (is_white(piece)) {
            printf("Move invalidated - not a black piece!\n");
            return 0;
        }
    }

    // check if move is valid for that piece
    if (!is_valid_piece_move(game, move)) {
        printf("Move invalidated - not a valid piece move!\n");
        return 0;
    }

    // check if move goes through pieces or lands on own piece (not allowed!)
    if (!is_move_unobstructed(game, move)) {
        printf("Move invalidated - move obstructed!\n");
        return 0;
    }

    // check if move is puts self in check (which would be illegal)
    Game *clone = clone_game(game);
    move_piece(clone, move);
    if (is_in_check(clone, active_player)) {
        delete_game(clone);
        printf("Move invalidated - move places king in check!\n");
        return 0;
    }
    delete_game(clone);


    return 1;
}


uint8_t is_in_check(Game *game, uint8_t player) {
    uint8_t king_idx;
    if (player == PLAYERW) {
        for (int i = 0; i < 64; i++) {
            if (game->board[i] == (KING | WHITE)) {
                king_idx = i;
                break;
            }
        }
    }
    else {
        for (int i = 0; i < 64; i++) {
            if (game->board[i] == (KING | BLACK)) {
                king_idx = i;
                break;
            }
        }
    }

    if (player == PLAYERW) {
        for (int i = 0; i < 64; i++) {
            if (game->board[i] & BLACK) {
                if (is_attacking(game, king_idx, i)) {
                    return 1;
                }
            }
        }
    }
    else {
        for (int i = 0; i < 64; i++) {
            if (game->board[i] & WHITE) {
                if (is_attacking(game, king_idx, i)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}


uint8_t is_valid_piece_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    // uint8_t to = (move >> 8) & 0xFF;
    
    uint8_t piece = game->board[from];

    if (is_pawn(piece)) {
        return is_valid_pawn_move(game, move);
    }
    else if (is_knight(piece)) {
        return is_valid_knight_move(game, move);
    }
    else if (is_bishop(piece)) {
        return is_valid_bishop_move(game, move);
    }
    else if (is_rook(piece)) {
        return is_valid_rook_move(game, move);
    }
    else if (is_queen(piece)) {
        // easy hack instead of making a queen move validator
        return (is_valid_bishop_move(game, move) || is_valid_rook_move(game, move));
    }
    // assumes the piece is a king if nothing else matche
    else { 
        return is_valid_king_move(game, move);
    }
}


uint8_t is_move_unobstructed(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    // checks if the move should even be considered
    int8_t delta = to - from;
    uint8_t start_row_idx = from / 8;
    uint8_t end_row_idx = to / 8;
    if (!(delta % 7 == 0 || delta % 9 == 0 || delta % 8 == 0 || start_row_idx == end_row_idx)) {
        return 1;
    }

    uint8_t start = from;
    uint8_t end = to;

    if (to < from) {
        start = to;
        end = from;
    }

    // pawn specific checks
    if ((game->board[from] & PAWN) == PAWN) {
        if (game->board[to] != NONE) {
            return 0;
        }
    }

    // up-left and down-right diagonals
    if (delta % 7 == 0) {
        for (int i = start + 7; i < end; i += 7) {
            if (game->board[i] != 0) {
                return 0;
            }
        }
    }
    // up-right and down-left diagonals
    else if (delta % 9 == 0) {
        for (int i = start + 9; i < end; i += 9) {
            if (game->board[i] != 0) {
                return 0;
            }
        }
    }
    // vertical sliding
    else if (delta % 8 == 0) {
        for (int i = start + 8; i < end; i += 8) {
            if (game->board[i] != 0) {
                return 0;
            }
        }
    }
    // horizontal sliding
    else {
        for (int i = start + 1; i < end; i++) {
            if (game->board[i] != 0) {
                return 0;
            }
        }

    }


    return 1;
}


uint8_t is_attacking(Game *game, uint8_t target, uint8_t attacker) {
    uint32_t speculative_move = attacker + (target << 8);

    if (!is_valid_piece_move(game, speculative_move)) {
        return 0;
    }

    if (!is_move_unobstructed(game, speculative_move)) {
        return 0;
    }

    return 1;
}


// piece move validator functions
uint8_t is_valid_pawn_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    uint8_t piece = game->board[from];
    uint8_t first_move = 0;
    
    if (is_white(piece) && (from / 8) == 1) {
        first_move = 1;
    }
    else if (!is_white(piece) && (from / 8) == 6) {
        first_move = 1;
    }

    if (is_white(piece)) {
        if (!((to - from) == 8 || ((to - from) == 16 && first_move))
                && !(((to - from) == 7) && (((game->board[to] & BLACK) == BLACK) 
                                           || game->w_en_passant_square == to))
                && !(((to - from) == 9) && (((game->board[to] & BLACK) == BLACK) 
                                           || game->w_en_passant_square == to))) {
            return 0;
        }
    }
    else {
        if (!((to - from) == -8 || ((to - from) == -16 && first_move))
                && !(((to - from) == -7) && (((game->board[to] & WHITE) == WHITE) 
                                           || game->b_en_passant_square == to))
                && !(((to - from) == -9) && (((game->board[to] & WHITE) == WHITE) 
                                           || game->b_en_passant_square == to))) {
            return 0;
        }

    }

    return 1;
}


uint8_t is_valid_knight_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    // account for negatives
    int8_t delta = to - from;
    if (delta < 0) {
        delta = -delta;
    }

    // possible moves for knight (positive only since negatives eliminated)
    if (!(delta == 6 || delta == 10 || delta == 15 || delta == 17)) {
        return 0;
    }

    return 1;
}


uint8_t is_valid_bishop_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    int8_t delta = to - from;

    // don't need to correct negative deltas due to modulo properties
    if (!(delta % 7 == 0 || delta % 9 == 0)) {
        return 0;
    }

    return 1;
}


uint8_t is_valid_rook_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    int8_t delta = to - from;

    // don't need to correct negative deltas due to modulo properties
    if (!(delta % 8 == 0)) {
        return 0;
    }

    // account for same row movement
    uint8_t start_row_idx = from / 8;
    uint8_t end_row_idx = to / 8;
    if (!(start_row_idx == end_row_idx)) {
        return 0;
    }

    return 1;
}


uint8_t is_valid_queen_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    int8_t delta = to - from;

    // don't need to correct negative deltas due to modulo properties
    if (!(delta % 7 == 0 || delta % 9 == 0 || delta % 8 == 0)) {
        return 0;
    }
    
    // account for same row movement
    uint8_t start_row_idx = from / 8;
    uint8_t end_row_idx = to / 8;
    if (!(start_row_idx == end_row_idx)) {
        return 0;
    }

    return 1;
    
}


uint8_t is_valid_king_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    int8_t delta = to - from;
    if (delta < 0) {
        delta = -delta;
    }

    if (!(delta == 1 || delta == 7 || delta == 8 || delta == 9)) {
        return 0;
    }

    return 1;
}
