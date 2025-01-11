#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "player.h"
#include "piece.h"
#include "move.h"
#include "io.h"

Game *create_game() {
    Game *game = (Game *) malloc(sizeof(Game));
    game->player_w = USER;
    game->player_b = USER;
    
    game->move = 0;
    game->castle_kingside_w = 1;
    game->castle_queenside_w = 1;
    game->castle_kingside_b = 1;
    game->castle_queenside_b = 1;

    game->w_en_passant_square = 0xFF;
    game->b_en_passant_square = 0xFF;

    for (int i = 0; i < 64; i++) {
        game->board[i] = NONE;
    }

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

    clone->w_en_passant_square = game->w_en_passant_square;
    clone->b_en_passant_square = game->b_en_passant_square;

    return clone;
}	


void delete_game(Game *game) {
    if (!game) {
        return;
    }

    free(game);
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

    game->board[0] = WHITE | ROOK;
    game->board[1] = WHITE | KNIGHT;
    game->board[2] = WHITE | BISHOP;
    game->board[3] = WHITE | QUEEN;
    game->board[4] = WHITE | KING;
    game->board[5] = WHITE | BISHOP;
    game->board[6] = WHITE | KNIGHT;
    game->board[7] = WHITE | ROOK;

    game->board[56] = BLACK | ROOK;
    game->board[57] = BLACK | KNIGHT;
    game->board[58] = BLACK | BISHOP;
    game->board[59] = BLACK | QUEEN;
    game->board[60] = BLACK | KING;
    game->board[61] = BLACK | BISHOP;
    game->board[62] = BLACK | KNIGHT;
    game->board[63] = BLACK | ROOK;
}


void move_piece(Game *game, uint32_t move) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    // en passant targeting
    if ((game->board[from] & PAWN)) {
        int8_t delta = to - from;
        if (move & MOVE_WHITE_MASK) {
            if (delta == 16) {
                game->b_en_passant_square = from + 8;
            }
            else {
                game->b_en_passant_square = 0xFF;
            }
        }
        else {
            if (delta == -16) {
                game->w_en_passant_square = from - 8;
            }
            else {
                game->w_en_passant_square = 0xFF;
            }
        }
    }

    // en passant attacking
    if ((game->board[from] & PAWN)) {
        if (move & MOVE_WHITE_MASK) {
            if (game->w_en_passant_square == to) {
                game->board[to - 8] = NONE;
            }
        }
        else {
            if (game->b_en_passant_square == to) {
                game->board[to + 8] = NONE;
            }
        }
    }


    // end changes
    game->board[to] = game->board[from];

    game->board[from] = NONE;
}


void print_board(Game *game) {
    for (int i = 7; i >= 0; i--) {
        printf("  +----+----+----+----+----+----+----+----+\n");
        printf("%d |", i + 1);

        char piece_str[3];
        for (int j = 0; j < 8; j++) {
            uint8_t piece = game->board[(i * 8) + j];
            get_piece_str(piece, piece_str);
            printf(" %s |", piece_str);
        }
        printf("\n");

    }
    printf("  +----+----+----+----+----+----+----+----+\n");
    printf("    A    B    C    D    E    F    G    H\n");
}


void print_board_reverse(Game *game) {
    for (int i = 0; i < 8; i++) {
        printf("  +----+----+----+----+----+----+----+----+\n");
        printf("%d |", i + 1);

        char piece_str[3];
        for (int j = 7; j >= 0; j--) {
            uint8_t piece = game->board[(i * 8) + j];
            get_piece_str(piece, piece_str);
            printf(" %s |", piece_str);
        }
        printf("\n");

    }
    printf("  +----+----+----+----+----+----+----+----+\n");
    printf("    H    G    F    E    D    C    B    A\n");
}
