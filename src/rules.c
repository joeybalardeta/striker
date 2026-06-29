#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "rules.h"
#include "move.h"
#include "movelist.h"
#include "game.h"
#include "player.h"
#include "piece.h"
#include "utils.h"

// debug define macros (enables compilation of printf statements)
// #define DEBUG_CHECK
// #define DEBUG_MOVE_INVALIDATION
// #define DEBUG_CASTLING

// shared (row, col) step tables for ray/offset based scanning
static const int8_t DIR_ORTHO[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
static const int8_t DIR_DIAG[4][2]  = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
static const int8_t KNIGHT_OFFSETS[8][2] = {
    {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
    {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
};

// the legality checks that don't depend on king safety (piece present, owned
// by the side to move, not landing on a friendly piece, geometrically valid
// and unobstructed)
uint8_t is_pseudo_legal(Game *game, uint32_t move, uint8_t active_player, uint8_t active_color) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    uint8_t piece = game->board[from];
    if (!is_piece(piece)) {
        return 0;
    }

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

    if (game->board[to] & active_color) {       // lands on own piece
        return 0;
    }
    if (!is_valid_piece_move(game, move)) {
        return 0;
    }
    if (!is_move_unobstructed(game, move)) {
        return 0;
    }

    return 1;
}


// returns 1 if making 'move' leaves the side-to-move's king safe. castling
// relocates the rook, so it is tested on a clone; every other move uses a cheap
// in-place board mutation + undo (no full Game copy).
// king_sq is the side-to-move's king square before the move (0xFF if unknown,
// in which case it is looked up); the caller passes it to avoid recomputation.
uint8_t move_leaves_king_safe(Game *game, uint32_t move, uint8_t king_sq) {
    uint8_t active_player = !game->move ? PLAYERW : PLAYERB;
    uint8_t active_color = !game->move ? WHITE : BLACK;
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    if (move & (MOVE_KSC_FLAG_MASK | MOVE_QSC_FLAG_MASK)) {
        Game clone;
        memcpy(&clone, game, sizeof(Game));
        move_piece(&clone, move);
        return !is_in_check(&clone, active_player);
    }

    uint8_t moving = game->board[from];
    uint8_t captured = game->board[to];

    // detect en passant: a pawn moving diagonally onto the empty target square
    uint8_t ep_pawn_sq = 0xFF;
    uint8_t ep_pawn = NONE;
    if (is_pawn(moving) && (from % 8) != (to % 8) && captured == NONE) {
        ep_pawn_sq = (active_color == WHITE) ? (to - 8) : (to + 8);
        ep_pawn = game->board[ep_pawn_sq];
    }

    // apply in place
    game->board[to] = moving;
    game->board[from] = NONE;
    if (ep_pawn_sq != 0xFF) {
        game->board[ep_pawn_sq] = NONE;
    }

    // king square after the move (use 'to' for a king move; else the cached square)
    uint8_t king_idx = is_king(moving) ? to
        : (king_sq != 0xFF ? king_sq : get_king_square(game, active_player));
    uint32_t attacking_color = active_color == WHITE ? BLACK : WHITE;
    uint8_t safe = (king_idx == 0xFF)
        || !is_square_attacked(game, king_idx / 8, king_idx % 8, attacking_color);

    // undo
    game->board[from] = moving;
    game->board[to] = captured;
    if (ep_pawn_sq != 0xFF) {
        game->board[ep_pawn_sq] = ep_pawn;
    }

    return safe;
}


uint8_t is_legal_move(Game *game, uint32_t move) {
    uint8_t active_player = !game->move ? PLAYERW : PLAYERB;
    uint8_t active_color = !game->move ? WHITE : BLACK;

    if (!is_pseudo_legal(game, move, active_player, active_color)) {
        return 0;
    }

    return move_leaves_king_safe(game, move, get_king_square(game, active_player));
}


// directional ray/offset scan outward from a square, instead of testing
// every enemy piece on the board. returns 1 if (kr, kc) is attacked by
// attacking_color.
uint8_t is_square_attacked(Game *game, int8_t kr, int8_t kc, uint32_t attacking_color) {

    // sliding attackers along ranks/files: rook or queen
    for (int d = 0; d < 4; d++) {
        int8_t r = kr + DIR_ORTHO[d][0];
        int8_t c = kc + DIR_ORTHO[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t piece = game->board[r * 8 + c];
            if (piece) {
                if ((piece & attacking_color) && (is_rook(piece) || is_queen(piece))) {
                    return 1;
                }
                break;  // first piece blocks the ray
            }
            r += DIR_ORTHO[d][0];
            c += DIR_ORTHO[d][1];
        }
    }

    // sliding attackers along diagonals: bishop or queen
    for (int d = 0; d < 4; d++) {
        int8_t r = kr + DIR_DIAG[d][0];
        int8_t c = kc + DIR_DIAG[d][1];
        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t piece = game->board[r * 8 + c];
            if (piece) {
                if ((piece & attacking_color) && (is_bishop(piece) || is_queen(piece))) {
                    return 1;
                }
                break;
            }
            r += DIR_DIAG[d][0];
            c += DIR_DIAG[d][1];
        }
    }

    // knight attackers
    for (int d = 0; d < 8; d++) {
        int8_t r = kr + KNIGHT_OFFSETS[d][0];
        int8_t c = kc + KNIGHT_OFFSETS[d][1];
        if (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t piece = game->board[r * 8 + c];
            if ((piece & attacking_color) && is_knight(piece)) {
                return 1;
            }
        }
    }

    // adjacent enemy king
    for (int8_t dr = -1; dr <= 1; dr++) {
        for (int8_t dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            int8_t r = kr + dr;
            int8_t c = kc + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint8_t piece = game->board[r * 8 + c];
                if ((piece & attacking_color) && is_king(piece)) {
                    return 1;
                }
            }
        }
    }

    // pawn attackers: white pawns attack from the row below the king,
    // black pawns from the row above, on the adjacent files
    int8_t pawn_row = (attacking_color == WHITE) ? (kr - 1) : (kr + 1);
    if (pawn_row >= 0 && pawn_row < 8) {
        for (int8_t dc = -1; dc <= 1; dc += 2) {
            int8_t c = kc + dc;
            if (c >= 0 && c < 8) {
                uint8_t piece = game->board[pawn_row * 8 + c];
                if ((piece & attacking_color) && is_pawn(piece)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}


uint8_t is_in_check(Game *game, uint8_t player) {
    uint32_t attacking_color = player == PLAYERW ? BLACK : WHITE;

    uint8_t king_idx = get_king_square(game, player);
    if (king_idx == 0xFF) {
        return 0;
    }

    return is_square_attacked(game, king_idx / 8, king_idx % 8, attacking_color);
}


uint8_t is_checkmate(Game *game) {
    uint8_t player = !game->move ? PLAYERW : PLAYERB;

    MoveList *possible_moves = get_possible_moves(game);
    uint16_t moves = possible_moves->length;
    delete_movelist(possible_moves);

    if (moves == 0 && is_in_check(game, player)) {
        return CHECKMATE;
    }
    return 0;
}


uint8_t is_draw(Game *game) {
    uint8_t player = !game->move ? PLAYERW : PLAYERB;

    MoveList *possible_moves = get_possible_moves(game);
    uint16_t moves = possible_moves->length;
    delete_movelist(possible_moves);

    // stalemate
    if (moves == 0 && is_in_check(game, player)) {
        return STALEMATE;
    }

    // draw by insufficient material
    uint8_t pc_w = 0;   // piece count white
    uint8_t pc_b = 0;   // piece count black
    for (int i = 0; i < BOARD_SIZE; i++) {
        uint8_t piece = game->board[i];
        if (piece & WHITE) {
            if (is_pawn(piece)) {
                pc_w += 2;
            }
            else if (is_knight(piece)) {
                pc_w += 1;
            }
            else if (is_bishop(piece)) {
                pc_w += 1;
            }
            else if (is_rook(piece)) {
                pc_w += 2;
            }
            else if (is_queen(piece)) {
                pc_w += 2;
            }
        }
        if (piece & BLACK) {
            if (is_pawn(piece)) {
                pc_b += 2;
            }
            else if (is_knight(piece)) {
                pc_b += 1;
            }
            else if (is_bishop(piece)) {
                pc_b += 1;
            }
            else if (is_rook(piece)) {
                pc_b += 2;
            }
            else if (is_queen(piece)) {
                pc_b += 2;
            }
        }

        if ((pc_w > 1) || (pc_b > 1)) {
            break;
        }
    }

    if ((pc_w < 2) && (pc_b < 2)) {
        return DRAW_IM;
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
    // assumes the piece is a king if nothing else matches
    else { 
        return is_valid_king_move(game, move);
    }
}


uint8_t is_move_unobstructed(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    // classify the move by row/column geometry (not by linear-index modular
    // arithmetic, which can misclassify a full-file slide as a diagonal)
    int8_t fr = from / 8, fc = from % 8;
    int8_t tr = to / 8, tc = to % 8;
    int8_t dr = tr - fr;
    int8_t dc = tc - fc;

    int8_t adr = dr < 0 ? -dr : dr;
    int8_t adc = dc < 0 ? -dc : dc;

    // pawn push: the destination square must be empty
    if (is_pawn(game->board[from]) && dc == 0 && game->board[to] != NONE) {
        return 0;
    }

    // only straight lines and true diagonals can be obstructed; anything else
    // (e.g. a knight's leap) has no squares in between
    if (!(dr == 0 || dc == 0 || adr == adc)) {
        return 1;
    }

    int8_t step_r = (dr > 0) - (dr < 0);
    int8_t step_c = (dc > 0) - (dc < 0);

    // walk the squares strictly between 'from' and 'to'
    int8_t r = fr + step_r;
    int8_t c = fc + step_c;
    while (r != tr || c != tc) {
        if (game->board[r * 8 + c] != NONE) {
            return 0;
        }
        r += step_r;
        c += step_c;
    }

    return 1;
}


// checks if a piece (on 'attacker' square) is attacking a square (target)
uint8_t is_piece_attacking(Game *game, uint8_t attacker, uint8_t target) {
    if (attacker == target) {
        return 0;
    }

    uint32_t speculative_move = attacker + (target << 8);

    if (!is_valid_piece_move(game, speculative_move)) {
        // printf("Piece is not attacking, invalid piece move.\n");
        return 0;
    }

    if (!is_move_unobstructed(game, speculative_move)) {
        // printf("Piece is not attacking, obstructed move.\n");
        return 0;
    }

    return 1;
}


// checks if square (target) is under attack by attacking color's pieces
uint8_t is_attacking(Game *game, uint8_t attacking_player, uint8_t target) {
    uint8_t attacking_color = attacking_player == PLAYERW ? WHITE : BLACK;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (game->board[i] & attacking_color) {
            if (is_piece_attacking(game, i, target)) {
                return 1;
            }
        }
    }

    // temporary pawn diagonal fix
    if (attacking_player == PLAYERB) {
        if ((game->board[target + 7] & attacking_color) && is_pawn(game->board[target + 7])
            && (target % 8 != 0)) {
            return 1;
        }
        if ((game->board[target + 9] & attacking_color) && is_pawn(game->board[target + 9])
            && (target % 8 != 7)) {
            return 1;
        }
    }
    else {
        if ((game->board[target - 7] & attacking_color) && is_pawn(game->board[target - 7])
            && (target % 8 != 7)) {
            return 1;
        }
        if ((game->board[target - 9] & attacking_color) && is_pawn(game->board[target - 9])
            && (target % 8 != 0)) {
            return 1;
        }
    }


    return 0;
}


// return first found 'attacking square' for a given target square
// attacking square = square that attacking piece is on
uint8_t get_attacking_square(Game *game, uint8_t attacking_player, uint8_t target) {
    uint8_t attacking_color = attacking_player == PLAYERW ? WHITE : BLACK;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (game->board[i] & attacking_color) {
            if (is_piece_attacking(game, i, target)) {
                return i;
            }
        }
    }

    // temporary pawn diagonal fix
    if (attacking_player == PLAYERB) {
        if ((game->board[target + 7] & attacking_color) && is_pawn(game->board[target + 7])
            && (target % 8 != 0)) {
            return target + 7;
        }
        if ((game->board[target + 9] & attacking_color) && is_pawn(game->board[target + 9])
            && (target % 8 != 7)) {
            return target + 9;
        }
    }
    else {
        if ((game->board[target - 7] & attacking_color) && is_pawn(game->board[target - 7])
            && (target % 8 != 7)) {
            return target - 7;
        }
        if ((game->board[target - 9] & attacking_color) && is_pawn(game->board[target - 9])
            && (target % 8 != 0)) {
            return target - 9;
        }
    }


    return 0xFF;
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

    uint8_t from_col = from % 8;

    // this looks terrible, should change it later
    if (is_white(piece)) {
        if (!((((to - from) == 8) && game->board[to] == NONE) || (((to - from) == 16 && first_move) && game->board[to] == NONE))
                && !(((to - from) == 7) && (((game->board[to] & BLACK) == BLACK) 
                                           || game->w_en_passant_square == to)
                    && from_col != 0)
                && !(((to - from) == 9) && (((game->board[to] & BLACK) == BLACK) 
                                           || game->w_en_passant_square == to)
                    && from_col != 7)) {
            return 0;
        }
    }
    else {
        if (!((((to - from) == -8) && game->board[to] == NONE) || (((to - from) == -16 && first_move) && game->board[to] == NONE))
                && !(((to - from) == -7) && (((game->board[to] & WHITE) == WHITE) 
                                           || game->b_en_passant_square == to)
                    && from_col != 7)
                && !(((to - from) == -9) && (((game->board[to] & WHITE) == WHITE) 
                                           || game->b_en_passant_square == to)
                    && from_col != 0)) {
            return 0;
        }

    }

    return 1;
}


uint8_t is_valid_knight_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    int8_t delta = to - from;

    uint8_t from_col = from % 8;

    // possible moves for knight
    if (!((from_col > 1 && (delta == 6 || delta == -10)) 
       || (from_col > 0 && (delta == 15 || delta == -17)) 
       || (from_col < 7 && (delta == 17 || delta == -15))
       || (from_col < 6 && (delta == 10 || delta == -6)))) {
        return 0;
    }

    return 1;
}


uint8_t is_valid_bishop_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    int8_t delta = to - from;
    if (from > to) {
        delta = -delta;
    }

    
    uint8_t diagonal = 0;
    uint8_t diagonal_squares = 0;
    
    if (delta % 9 == 0) {
        diagonal = 9;
        diagonal_squares = delta / 9;
    }
    else if (delta % 7 == 0) {
        diagonal = 7;
        diagonal_squares = delta / 7;
    }
    else {
        return 0;
    }

    uint8_t from_col = from % 8;

    if (to > from) {
        if (diagonal == 7 && !(from_col >= diagonal_squares)) {
            return 0;
        }
        else if (diagonal == 9 && !(from_col <= 7 - diagonal_squares)) {
            return 0;
        }
    }
    else {
        if (diagonal == 9 && !(from_col >= diagonal_squares)) {
            return 0;
        }
        else if (diagonal == 7 && !(from_col <= 7 - diagonal_squares)) {
            return 0;
        }
    }



    return 1;
}


uint8_t is_valid_rook_move(Game *game, uint32_t move) {
    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    int8_t delta = to - from;

    // account for same row movement
    uint8_t start_row_idx = from / 8;
    uint8_t end_row_idx = to / 8;
    
    // don't need to correct negative deltas due to modulo properties
    if (!(delta % 8 == 0 || start_row_idx == end_row_idx)) {
        return 0;
    }

    return 1;
}


uint8_t is_valid_queen_move(Game *game, uint32_t move) {
    if (!(is_valid_bishop_move(game, move) || is_valid_rook_move(game, move))) {
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

    uint8_t start_row = from / 8;
    uint8_t end_row = to / 8;

    if (!((delta == 1 && start_row == end_row) || delta == 7 || delta == 8 || delta == 9)) {
        // castling moves
        if (delta == 2 && start_row == end_row) {
            uint8_t player = (move & MOVE_WHITE_MASK) ? PLAYERW : PLAYERB;
            if (to > from && can_castle(game, player, 0)) {
                return 1;
            }
            else if (to < from && can_castle(game, player, 1)) {
                return 1;
            }
        }
        return 0;
    }

    return 1;
}


// extra
uint32_t add_move_flags(Game *game, uint32_t move) {
    // post move flag edits (more should be added here)
    if (!game->move) {
        move |= MOVE_WHITE_MASK;
    }
    else {
        move |= MOVE_BLACK_MASK;
    }

    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    if (is_king(game->board[from])) {
        if ((to - from) == 2) {
            move |= MOVE_KSC_FLAG_MASK;
        }
        else if ((from - to) == 2) {
            move |= MOVE_QSC_FLAG_MASK;
        }
    }

    return move;
}


uint8_t get_king_square(Game *game, uint8_t player) {
    uint8_t color = player == PLAYERW ? WHITE : BLACK;

    if ((game->king_square_w == 0xFF && color == WHITE)
        || (game->king_square_b == 0xFF && color == BLACK)) {
        for (int i = 0; i < BOARD_SIZE; i++) {
            if ((game->board[i] & color) && is_king(game->board[i])) {
                return i;
            }
        }
    }
    else {
        return color == WHITE ? game->king_square_w : game->king_square_b;
    }

    return 0xFF;
}


uint8_t can_castle(Game *game, uint8_t player, uint8_t queenside) {
    // uint8_t castling_color = player == PLAYERW ? WHITE : BLACK;
    uint8_t opposing_player = player == PLAYERW ? PLAYERB : PLAYERW;

    #ifdef DEBUG_CASTLING
    printf("%s %s castle.\n", player == PLAYERW ? "White" : "Black", !queenside ? "kingside" : "queenside");
    #endif

    if (player == PLAYERW) {
        if (!queenside) {
            if (!game->castle_kingside_w) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, no permission.\n");
                #endif
                return 0;
            }
            
            // check for obstruction
            if (game->board[5] != NONE
                || game->board[6] != NONE) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                #endif
                return 0;
            }

            // check for intermediate squares being attacked
            if (is_attacking(game, opposing_player, 5)) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, attacking piece.\n");
                #endif
                return 0;
            }

        }
        else {
            if (!game->castle_queenside_w) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, no permission.\n");
                #endif
                return 0;
            }
            
            // check for obstruction
            if (game->board[1] != NONE
                || game->board[2] != NONE
                || game->board[3] != NONE) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                printf("1: %u | 2: %u | 3: %u\n", game->board[1], game->board[2], game->board[3]);
                #endif
                return 0;
            }

            // check for intermediate squares being attacked
            if (is_attacking(game, opposing_player, 3)) {
                return 0;
            }
        }
    }
    else {
        if (!queenside) {
            if (!game->castle_kingside_b) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, no permission.\n");
                #endif
                return 0;
            }
            
            // check for obstruction
            if (game->board[61] != NONE
                || game->board[62] != NONE) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                #endif
                return 0;
            }

            // check for intermediate squares being attacked
            if (is_attacking(game, opposing_player, 61)) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                #endif
                return 0;
            }
        }
        else {
            if (!game->castle_queenside_b) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, no permission.\n");
                #endif
                return 0;
            }
            
            // check for obstruction
            if (game->board[57] != NONE
                || game->board[58] != NONE
                || game->board[59] != NONE) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                #endif
                return 0;
            }

            // check for intermediate squares being attacked
            if (is_attacking(game, opposing_player, 59)) {
                #ifdef DEBUG_CASTLING
                printf("Cannot castle, obstructing piece(s).\n");
                #endif
                return 0;
            }
        }
    }
    
    // check if player is in check
    if (is_in_check(game, player)) {
        #ifdef DEBUG_CASTLING
        printf("Cannot castle, king in check.\n");
        #endif
        return 0;
    }

    #ifdef DEBUG_CASTLING
    printf("Can castle.\n");
    #endif
    return 1;
}


uint8_t is_pawn_promotion_move(Game *game, uint32_t move) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    if (is_pawn(game->board[from])) {
        if (move & MOVE_WHITE_MASK) {
            if ((to / 8) == 7) {
                return 1;
            }
        }
        else {
            if ((to / 8) == 0) {
                return 1;
            }
        }
    }

    return 0;
}


// fills `targets` with pseudo-legal destination squares for the piece on `from`:
// it excludes friendly-occupied squares, respects pawn push/capture rules and
// en passant, and stops slider rays at the first blocker (including it only if
// it is a capture). king-safety (check/pin) is decided by the caller; castling
// destinations are emitted unconditionally and validated by the caller via
// can_castle. returns the number of candidates written.
static uint8_t get_candidate_targets(Game *game, uint8_t from, uint8_t *targets) {
    uint8_t piece = game->board[from];
    uint8_t count = 0;
    int8_t fr = from / 8;
    int8_t fc = from % 8;
    uint32_t own = is_white(piece) ? WHITE : BLACK;
    uint32_t enemy = is_white(piece) ? BLACK : WHITE;

    if (is_pawn(piece)) {
        int8_t dir = is_white(piece) ? 1 : -1;
        int8_t start_rank = is_white(piece) ? 1 : 6;

        // single push (and double push from the start rank) onto empty squares
        if (fr + dir >= 0 && fr + dir < 8 && game->board[(fr + dir) * 8 + fc] == NONE) {
            targets[count++] = (fr + dir) * 8 + fc;
            if (fr == start_rank && game->board[(fr + 2 * dir) * 8 + fc] == NONE) {
                targets[count++] = (fr + 2 * dir) * 8 + fc;
            }
        }

        // diagonal captures, including en passant
        uint8_t ep_sq = (own == WHITE) ? game->w_en_passant_square
                                       : game->b_en_passant_square;
        for (int8_t dc = -1; dc <= 1; dc += 2) {
            int8_t r = fr + dir;
            int8_t c = fc + dc;
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint8_t sq = r * 8 + c;
                if ((game->board[sq] & enemy) || sq == ep_sq) {
                    targets[count++] = sq;
                }
            }
        }
    }
    else if (is_knight(piece)) {
        for (int d = 0; d < 8; d++) {
            int8_t r = fr + KNIGHT_OFFSETS[d][0];
            int8_t c = fc + KNIGHT_OFFSETS[d][1];
            if (r >= 0 && r < 8 && c >= 0 && c < 8 && !(game->board[r * 8 + c] & own)) {
                targets[count++] = r * 8 + c;
            }
        }
    }
    else if (is_king(piece)) {
        // adjacent squares not occupied by a friendly piece
        for (int8_t dr = -1; dr <= 1; dr++) {
            for (int8_t dc = -1; dc <= 1; dc++) {
                if (dr == 0 && dc == 0) {
                    continue;
                }
                int8_t r = fr + dr;
                int8_t c = fc + dc;
                if (r >= 0 && r < 8 && c >= 0 && c < 8 && !(game->board[r * 8 + c] & own)) {
                    targets[count++] = r * 8 + c;
                }
            }
        }
        // castling destinations (two squares along the rank), validated by caller
        if (fc - 2 >= 0) {
            targets[count++] = from - 2;
        }
        if (fc + 2 < 8) {
            targets[count++] = from + 2;
        }
    }
    else {
        // sliding pieces: walk rays until the board edge or first blocker
        uint8_t do_ortho = is_rook(piece) || is_queen(piece);
        uint8_t do_diag  = is_bishop(piece) || is_queen(piece);

        if (do_ortho) {
            for (int d = 0; d < 4; d++) {
                int8_t r = fr + DIR_ORTHO[d][0];
                int8_t c = fc + DIR_ORTHO[d][1];
                while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                    uint8_t sq = r * 8 + c;
                    uint8_t occ = game->board[sq];
                    if (occ) {
                        if (occ & enemy) {
                            targets[count++] = sq;      // capture the blocker
                        }
                        break;
                    }
                    targets[count++] = sq;
                    r += DIR_ORTHO[d][0];
                    c += DIR_ORTHO[d][1];
                }
            }
        }
        if (do_diag) {
            for (int d = 0; d < 4; d++) {
                int8_t r = fr + DIR_DIAG[d][0];
                int8_t c = fc + DIR_DIAG[d][1];
                while (r >= 0 && r < 8 && c >= 0 && c < 8) {
                    uint8_t sq = r * 8 + c;
                    uint8_t occ = game->board[sq];
                    if (occ) {
                        if (occ & enemy) {
                            targets[count++] = sq;
                        }
                        break;
                    }
                    targets[count++] = sq;
                    r += DIR_DIAG[d][0];
                    c += DIR_DIAG[d][1];
                }
            }
        }
    }

    return count;
}


// single pass around king_sq that computes both whether the king is in check
// and which own pieces are pinned -- the slider rays are shared between the two
// (a ray's first piece can be a checker; first-own-then-enemy-slider is a pin).
// pinned pieces are written to the parallel arrays pin_sq/pin_dr/pin_dc (at most
// 8); the outward king->piece step is the pin axis. returns the number of pins
// and sets *out_in_check.
static uint8_t compute_king_context(Game *game, uint8_t king_sq, uint32_t own_color,
                                    uint32_t enemy_color, uint8_t *out_in_check,
                                    uint8_t *pin_sq, int8_t *pin_dr, int8_t *pin_dc) {
    int8_t kr = king_sq / 8;
    int8_t kc = king_sq % 8;
    uint8_t check = 0;
    uint8_t npins = 0;

    for (int d = 0; d < 8; d++) {
        // first four directions orthogonal (rook/queen), last four diagonal (bishop/queen)
        uint8_t ortho = d < 4;
        int8_t ddr = ortho ? DIR_ORTHO[d][0] : DIR_DIAG[d - 4][0];
        int8_t ddc = ortho ? DIR_ORTHO[d][1] : DIR_DIAG[d - 4][1];

        int8_t r = kr + ddr;
        int8_t c = kc + ddc;
        int8_t cand = -1;       // first own piece encountered along the ray

        while (r >= 0 && r < 8 && c >= 0 && c < 8) {
            uint8_t pc = game->board[r * 8 + c];
            if (pc) {
                uint8_t slider = ortho ? (is_rook(pc) || is_queen(pc))
                                       : (is_bishop(pc) || is_queen(pc));
                if (cand < 0) {
                    if (pc & enemy_color) {
                        if (slider) {
                            check = 1;          // direct slider check
                        }
                        break;                  // enemy first piece: no pin past it
                    }
                    cand = r * 8 + c;           // own piece: candidate pinned
                }
                else {
                    if ((pc & enemy_color) && slider) {
                        pin_sq[npins] = cand;
                        pin_dr[npins] = ddr;
                        pin_dc[npins] = ddc;
                        npins++;
                    }
                    break;
                }
            }
            r += ddr;
            c += ddc;
        }
    }

    // knight attackers
    if (!check) {
        for (int d = 0; d < 8; d++) {
            int8_t r = kr + KNIGHT_OFFSETS[d][0];
            int8_t c = kc + KNIGHT_OFFSETS[d][1];
            if (r >= 0 && r < 8 && c >= 0 && c < 8) {
                uint8_t pc = game->board[r * 8 + c];
                if ((pc & enemy_color) && is_knight(pc)) {
                    check = 1;
                    break;
                }
            }
        }
    }

    // pawn attackers (enemy pawns attack diagonally toward the king)
    if (!check) {
        int8_t pawn_row = (enemy_color == WHITE) ? (kr - 1) : (kr + 1);
        if (pawn_row >= 0 && pawn_row < 8) {
            for (int8_t dc = -1; dc <= 1; dc += 2) {
                int8_t c = kc + dc;
                if (c >= 0 && c < 8) {
                    uint8_t pc = game->board[pawn_row * 8 + c];
                    if ((pc & enemy_color) && is_pawn(pc)) {
                        check = 1;
                        break;
                    }
                }
            }
        }
    }

    *out_in_check = check;
    return npins;
}


// fills a caller-provided list, so hot callers (perft/search) can use a
// stack-allocated MoveList and avoid a heap allocation per node
void generate_moves(Game *game, MoveList *possible_moves) {
    clear_movelist(possible_moves);

    uint8_t player = !game->move ? PLAYERW : PLAYERB;
    uint8_t active_color = player == PLAYERW ? WHITE : BLACK;
    uint32_t enemy_color = active_color == WHITE ? BLACK : WHITE;

    // king-safety context (check status + pinned pieces) computed once per node
    // in a single king scan, instead of per candidate move. pins are kept as a
    // short list (at most 8) so the common no-pin node does no extra work.
    uint8_t king_sq = get_king_square(game, player);
    uint8_t in_check = 0;
    uint8_t pin_sq[8];
    int8_t pin_dr[8];
    int8_t pin_dc[8];
    uint8_t npins = 0;
    if (king_sq != 0xFF) {
        npins = compute_king_context(game, king_sq, active_color, enemy_color,
                                     &in_check, pin_sq, pin_dr, pin_dc);
    }

    uint32_t color_flag = (player == PLAYERW) ? MOVE_WHITE_MASK : MOVE_BLACK_MASK;
    uint8_t targets[32];

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (game->board[i] & active_color) {
            uint8_t moving = game->board[i];
            // values that are constant for this piece are hoisted out of the
            // per-target loop below
            uint8_t moving_is_king = is_king(moving);
            uint8_t moving_is_pawn = is_pawn(moving);

            // is this piece pinned? (npins is almost always 0)
            int8_t pdr = 0;
            int8_t pdc = 0;
            for (uint8_t k = 0; k < npins; k++) {
                if (pin_sq[k] == i) {
                    pdr = pin_dr[k];
                    pdc = pin_dc[k];
                    break;
                }
            }

            // get_candidate_targets returns pseudo-legal destinations, so no
            // separate is_pseudo_legal pass is needed (castling is validated below)
            uint8_t ntargets = get_candidate_targets(game, i, targets);
            for (int t = 0; t < ntargets; t++) {
                uint8_t to = targets[t];
                uint32_t move = i | (to << 8) | color_flag;

                // detect castling (king two squares along the rank) and flag it
                uint8_t is_castle = 0;
                if (moving_is_king) {
                    if (to == i + 2) {
                        move |= MOVE_KSC_FLAG_MASK;
                        is_castle = 1;
                    }
                    else if (to + 2 == i) {
                        move |= MOVE_QSC_FLAG_MASK;
                        is_castle = 1;
                    }
                }

                uint8_t is_ep = moving_is_pawn && (i % 8 != to % 8)
                    && game->board[to] == NONE;

                uint8_t legal;
                if (is_castle) {
                    // cannot castle out of check; can_castle covers rights, empty
                    // path and the crossed square; move_leaves_king_safe the landing
                    legal = !in_check
                        && is_pseudo_legal(game, move, player, active_color)
                        && move_leaves_king_safe(game, move, king_sq);
                }
                else if (in_check || moving_is_king || is_ep) {
                    // king moves, en passant and check evasions need the full scan
                    legal = move_leaves_king_safe(game, move, king_sq);
                }
                else if (pdr || pdc) {
                    // a pinned piece may only move along its pin axis: the move
                    // displacement must be collinear with the pin direction
                    // (cross product zero). a sign test is not enough -- a knight
                    // leap can share the pin's signs without being on the line.
                    int8_t mdr = (int8_t)(to / 8) - (int8_t)(i / 8);
                    int8_t mdc = (int8_t)(to % 8) - (int8_t)(i % 8);
                    legal = (mdr * pdc - mdc * pdr) == 0;
                }
                else {
                    // not in check, not pinned, not a king/ep move: always safe
                    legal = 1;
                }

                if (!legal) {
                    continue;
                }

                // only a pawn reaching the far rank promotes
                if (!(moving_is_pawn && (to >= 56 || to < 8))) {
                    add_move(possible_moves, move);
                }
                else {
                    add_move(possible_moves, move | MOVE_NP_FLAG_MASK);
                    add_move(possible_moves, move | MOVE_BP_FLAG_MASK);
                    add_move(possible_moves, move | MOVE_RP_FLAG_MASK);
                    add_move(possible_moves, move | MOVE_QP_FLAG_MASK);
                }
            }
        }
    }
}


MoveList *get_possible_moves(Game *game) {
    MoveList *possible_moves = create_movelist();
    generate_moves(game, possible_moves);
    return possible_moves;
}
