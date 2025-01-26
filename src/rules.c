#include <stdio.h>
#include <stdint.h>
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

uint8_t is_legal_move(Game *game, uint32_t move) {
    // get the moving player
    uint8_t active_player = !game->move ? PLAYERW : PLAYERB;
    uint8_t active_color = !game->move ? WHITE : BLACK;

    // deserialize move
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;

    // check if piece is on from square
    uint8_t piece = game->board[from];
    if (!is_piece(piece)) {
        #ifdef DEBUG_MOVE_INVALIDATION
        printf("Move invalidated - no piece on from square!\n");
        #endif
        return 0;
    }

    // verify piece is active player's piece
    if (active_player == PLAYERW) {
        if (!is_white(piece)) {
            #ifdef DEBUG_MOVE_INVALIDATION
            printf("Move invalidated - not a white piece!\n");
            #endif
            return 0;
        }
    }
    else {
        if (is_white(piece)) {
            #ifdef DEBUG_MOVE_INVALIDATION
            printf("Move invalidated - not a black piece!\n");
            #endif
            return 0;
        }
    }

    // check if move lands on own piece (not allowed!)
    if (game->board[to] & active_color) {
        #ifdef DEBUG_MOVE_INVALIDATION
        printf("Move invalidated - move lands on own piece!\n");
        #endif
        return 0;
    }

    // check if move is valid for that piece
    if (!is_valid_piece_move(game, move)) {
        #ifdef DEBUG_MOVE_INVALIDATION
        printf("Move invalidated - not a valid piece move!\n");
        #endif
        return 0;
    }

    // check if move goes through pieces (not allowed!)
    if (!is_move_unobstructed(game, move)) {
        #ifdef DEBUG_MOVE_INVALIDATION
        printf("Move invalidated - move obstructed!\n");
        #endif
        return 0;
    }

    // check if move is puts self in check (which would be illegal)
    Game *clone = clone_game(game);
    move_piece(clone, move);
    if (is_in_check(clone, active_player)) {
        #ifdef DEBUG_MOVE_INVALIDATION
        print_board(clone);
        printf("Move invalidated - move places king in check!\n");
        uint8_t opposing_player = !game->move ? PLAYERB : PLAYERW;
        uint8_t king_square = get_king_square(clone, active_player);
        printf("Checking square: %u\n", get_attacking_square(clone, opposing_player, king_square));
        #endif
        
        delete_game(clone);
        return 0;
    }
    
    delete_game(clone);
    return 1;
}


uint8_t is_in_check(Game *game, uint8_t player) {
    uint32_t attacking_color;

    uint8_t king_idx;
    if (player == PLAYERW) {
        attacking_color = BLACK;
        for (int i = 0; i < 64; i++) {
            if (game->board[i] == (KING | WHITE)) {
                king_idx = i;
                break;
            }
        }
    }
    else {
        attacking_color = WHITE;
        for (int i = 0; i < 64; i++) {
            if (game->board[i] == (KING | BLACK)) {
                king_idx = i;
                break;
            }
        }
    }

    for (int i = 0; i < 64; i++) {
        #ifdef DEBUG_CHECK
        printf("Viewing square ");
        print_square(i);
        printf(" (%d | %p)  |  ", i, &game->board[i]);
        printf("Piece value: %u\n", game->board[i]);
        #endif
        if (game->board[i] & attacking_color) {
            #ifdef DEBUG_CHECK
            printf(attacking_color == WHITE ? "White" : "Black");
            printf(" piece on ");
            print_square(i);
            printf("\n");
            #endif
            if (is_piece_attacking(game, i, king_idx)) {
                #ifdef DEBUG_CHECK
                printf("    is attacking ");
                printf(attacking_color == WHITE ? "Black" : "White");
                printf(" King\n");
                #endif
                return 1;
            }
        }
    }


    return 0;
}


uint8_t is_checkmate(Game *game) {
    MoveList *possible_moves = get_possible_moves(game);
    uint16_t moves = possible_moves->length;
    delete_movelist(possible_moves);

    if (moves == 0) {
        return 1;
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

    // checks if the move can even be obstructed (basically if the move is a non-knight move)
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
    if (is_pawn(game->board[from]) && (delta == 8 || delta == 16)) {
        if (game->board[to] != NONE) {
            return 0;
        }
    }

    // up-left and down-right diagonals
    if (delta % 7 == 0 && start_row_idx != end_row_idx) {
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

    for (int i = 0; i < BOARD_SIZE; i++) {
        if ((game->board[i] & color) && is_king(game->board[i])) {
            return i;
        }
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


MoveList *get_possible_moves(Game *game) {
    MoveList *possible_moves = create_movelist();

    uint8_t player = !game->move ? PLAYERW : PLAYERB;
    uint32_t color = player == PLAYERW ? WHITE : BLACK;

    for (int i = 0; i < BOARD_SIZE; i++) {
        if (game->board[i] & color) {
            for (int j = 0; j < BOARD_SIZE; j++) {
                uint32_t move = i + (j << 8);
                move = add_move_flags(game, move);

                uint8_t legal = is_legal_move(game, move);
                if (legal) {
                    if (!is_pawn_promotion_move(game, move)) {
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

    return possible_moves;
}
