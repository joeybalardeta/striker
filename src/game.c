#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "game.h"
#include "player.h"
#include "piece.h"
#include "move.h"
#include "io.h"
#include "utils.h"

// debug define macros (enables compilation of printf statements)
// #define DEBUG_PTRS

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

    for (int i = 0; i < 64; i++) {
        clone->board[i] = game->board[i];
    }

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


void print_all_square_values(Game *game) {
    printf("Square values\n");
    for (int i = 0; i < 64; i++) {
        print_square(i);
        printf(": %u\n", game->board[i]);
    }
    printf("\n");
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
    
    if (move & MOVE_WHITE_MASK) {
        game->b_en_passant_square = 0xFF;
    }
    else {
        game->w_en_passant_square = 0xFF;
    }

    // en passant targeting
    if (is_pawn(game->board[from])) {
        int8_t delta = to - from;
        if (move & MOVE_WHITE_MASK) {
            if (delta == 16) {
                game->b_en_passant_square = from + 8;
            }
        }
        else {
            if (delta == -16) {
                game->w_en_passant_square = from - 8;
            }
        }
    }

    // en passant attacking
    if (is_pawn(game->board[from])) {
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

    // castling moves
    if (is_king(game->board[from]) && (move & MOVE_KSC_FLAG_MASK)) {
        game->board[from + 1] = game->board[from + 3];
        game->board[from + 3] = NONE;
    }
    else if (is_king(game->board[from]) && (move & MOVE_QSC_FLAG_MASK)) {
        game->board[from - 1] = game->board[from - 4];
        game->board[from - 4] = NONE;
    }

    modify_castling_rights(game, move);


    // pawn promotion handling
    uint32_t piece_color = (move & MOVE_WHITE_MASK ) ? WHITE : BLACK;
    if (MOVE_NP_FLAG_MASK) {
        game->board[from] = piece_color | KNIGHT;
    }
    else if (MOVE_BP_FLAG_MASK) {
        game->board[from] = piece_color | BISHOP;
    }
    else if (MOVE_RP_FLAG_MASK) {
        game->board[from] = piece_color | ROOK;
    }
    else if (MOVE_QP_FLAG_MASK) {
        game->board[from] = piece_color | QUEEN;
    }


    // end changes
    game->board[to] = game->board[from];

    game->board[from] = NONE;
}


void modify_castling_rights(Game *game, uint32_t move) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    
    uint8_t from_piece = game->board[from];
    
    if (game->castle_kingside_w == 0
        && game->castle_queenside_w == 0
        && game->castle_kingside_b == 0
        && game->castle_queenside_b == 0) {
        return;
    }

    // castling
    if ((move & MOVE_KSC_FLAG_MASK) || (move & MOVE_QSC_FLAG_MASK)) {
        if (move & MOVE_WHITE_MASK) {
            game->castle_kingside_w = 0;
            game->castle_queenside_w = 0;
        }
        else {
            game->castle_kingside_b = 0;
            game->castle_queenside_b = 0;
        }
        return;
    }

    // king move
    if ((move & MOVE_WHITE_MASK) && is_king(from_piece)) {
        game->castle_kingside_w = 0;
        game->castle_queenside_w = 0;
        return;
    }
    else if ((move & MOVE_BLACK_MASK) && is_king(from_piece)) {
        game->castle_kingside_b = 0;
        game->castle_queenside_b = 0;
        return;
    }

    // rook move
    if (is_rook(from_piece)) {
        if (move & MOVE_WHITE_MASK) {
            if (from == 7) {
                game->castle_kingside_w = 0;
            }
            else if (from == 0) {
                game->castle_queenside_w = 0;
            }
            return;
        }
        if (move & MOVE_BLACK_MASK) {
            if (from == 63) {
                game->castle_kingside_b = 0;
            }
            else if (from == 56) {
                game->castle_queenside_b = 0;
            }
            return;
        }
    }
    
    // rook capture moves (or they aren't there, and castling is already disabled)
    if (to == 7) {
        game->castle_kingside_w = 0;
    }
    else if (to == 0) {
        game->castle_queenside_w = 0;
    }
    else if (to == 63) {
        game->castle_kingside_b = 0;
    }
    else if (to == 56) {
        game->castle_queenside_b = 0;
    }
}


void print_board(Game *game) {
    #ifdef DEBUG_PTRS
    printf("Board (@ %p)\n\n", game->board);
    #endif
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


Game *load_fen_game(const char *filepath) {
    char buffer[256] = {0};

    load_fen(filepath, buffer);

    Game *game = create_game();

    uint8_t square = 56;
    uint8_t rank = 7;
    uint16_t index = 0;

    while (buffer[index] != ' ') {
        switch (buffer[index]) {
            case '1' ... '8':
                square += (buffer[index] - '0');
                break;

            case '/':
                rank -= 1;
                square = rank * 8;

                break;
            
            case 'P':
                game->board[square] = WHITE | PAWN;
                square += 1;
                break;

            case 'N':
                game->board[square] = WHITE | KNIGHT;
                square += 1;
                break;
            
            case 'B':
                game->board[square] = WHITE | BISHOP;
                square += 1;
                break;
            
            case 'R':
                game->board[square] = WHITE | ROOK;
                square += 1;
                break;
            
            case 'Q':
                game->board[square] = WHITE | QUEEN;
                square += 1;
                break;
            
            case 'K':
                game->board[square] = WHITE | KING;
                square += 1;
                break;

            case 'p':
                game->board[square] = BLACK | PAWN;
                square += 1;
                break;

            case 'n':
                game->board[square] = BLACK | KNIGHT;
                square += 1;
                break;
            
            case 'b':
                game->board[square] = BLACK | BISHOP;
                square += 1;
                break;
            
            case 'r':
                game->board[square] = BLACK | ROOK;
                square += 1;
                break;
            
            case 'q':
                game->board[square] = BLACK | QUEEN;
                square += 1;
                break;
            
            case 'k':
                game->board[square] = BLACK | KING;
                square += 1;
                break;
            
            default:
                break;
        }
        index++;
    }

    uint8_t space_count = 0;

    game->castle_kingside_w = 0;
    game->castle_queenside_w = 0;
    game->castle_kingside_b = 0;
    game->castle_queenside_b = 0;


    while (space_count < 3) {
        switch (buffer[index]) {
            case 'w':
                game->move = 0;
                break;
            
            case 'b':
                game->move = 1;
                break;
            
            case 'K':
                game->castle_kingside_w = 1;
                break;
            
            case 'Q':
                game->castle_queenside_w = 1;
                break;
            
            case 'k':
                game->castle_kingside_b = 1;
                break;
            
            case 'q':
                game->castle_queenside_b = 1;
                break;

            case ' ':
                space_count++;
                break;

            default:
                break;
        }
        index++;
    }

    uint8_t en_passant_file = buffer[index];
    index++;
    uint8_t en_passant_rank = buffer[index];

    if (en_passant_file != '-') {
        en_passant_file -= 'a';
        en_passant_file -= '1';

        uint8_t en_passant_target_square = (en_passant_file * 8) + en_passant_rank;

        if (!game->move) {
            game->b_en_passant_square = en_passant_target_square;
        }
        else {
            game->w_en_passant_square = en_passant_target_square;
        }
        index++;
    }

    index++;


    uint16_t half_moves_count = buffer[index];
    index++;
    uint16_t full_moves_count = buffer[index];

    return game;

}
