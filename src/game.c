#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
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

    game->king_square_w = 0xFF;
    game->king_square_b = 0xFF;

    for (int i = 0; i < 64; i++) {
        game->board[i] = NONE;
    }

    // start with empty, consistent bitboards (board is all NONE)
    rebuild_bitboards(game);

    return game;
}


// Derives all bitboards (per-piece, per-color occupancy, and combined
// occupancy) from the mailbox board. Used after board setup / FEN parsing so
// the two representations always start in agreement.
void rebuild_bitboards(Game *game) {
    int color;      // color index while zeroing
    int type;       // piece type index while zeroing
    int sq;         // square being scanned
    uint8_t piece;  // piece code on the square
    Bitboard bit;   // that square's bit

    // clear every bitboard first
    for (color = 0; color < 2; color++) {
        game->occ[color] = 0;
        for (type = 0; type < 7; type++) {
            game->pieces[color][type] = 0;
        }
    }
    game->occ_all = 0;

    // set one bit per occupied square, indexed by the piece's color and type
    for (sq = 0; sq < BOARD_SIZE; sq++) {
        piece = game->board[sq];
        if (piece != NONE) {
            color = (piece & BLACK) ? BB_BLACK : BB_WHITE;
            type = PIECE_MASK(piece);
            bit = BB_SQ(sq);
            game->pieces[color][type] |= bit;
            game->occ[color] |= bit;
            game->occ_all |= bit;
        }
    }
} /* rebuild_bitboards */


// Removes whatever piece sits on 'sq' from both the mailbox and the bitboards.
// A no-op on an empty square.
static void square_clear(Game *game, uint8_t sq) {
    uint8_t piece;  // piece currently on the square
    int color;      // its color index
    int type;       // its piece type
    Bitboard bit;   // its bit

    piece = game->board[sq];
    if (piece != NONE) {
        color = (piece & BLACK) ? BB_BLACK : BB_WHITE;
        type = PIECE_MASK(piece);
        bit = BB_SQ(sq);
        game->pieces[color][type] &= ~bit;
        game->occ[color] &= ~bit;
        game->occ_all &= ~bit;
        game->board[sq] = NONE;
    }
} /* square_clear */


// Places 'piece' on 'sq' in both the mailbox and the bitboards. Assumes the
// square has already been cleared.
static void square_set(Game *game, uint8_t sq, uint8_t piece) {
    int color;      // color index of the piece
    int type;       // its piece type
    Bitboard bit;   // its bit

    color = (piece & BLACK) ? BB_BLACK : BB_WHITE;
    type = PIECE_MASK(piece);
    bit = BB_SQ(sq);
    game->pieces[color][type] |= bit;
    game->occ[color] |= bit;
    game->occ_all |= bit;
    game->board[sq] = piece;
} /* square_set */


Game *clone_game(Game *game) {
    Game *clone = malloc(sizeof(Game));
    memcpy(clone, game, sizeof(Game));
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

    // track kings and derive the bitboards from the freshly filled mailbox
    game->king_square_w = 4;
    game->king_square_b = 60;
    rebuild_bitboards(game);
}


void move_piece(Game *game, uint32_t move) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    uint8_t piece = game->board[from];              // the moving piece
    uint32_t piece_color = (move & MOVE_WHITE_MASK) ? WHITE : BLACK;
    uint8_t rook;                                   // relocated rook when castling
    uint8_t landing = piece;                        // piece that ends up on 'to'

    // the side that just moved clears the opponent's stale en passant target
    if (move & MOVE_WHITE_MASK) {
        game->b_en_passant_square = 0xFF;
    }
    else {
        game->w_en_passant_square = 0xFF;
    }

    // en passant targeting (double push) and attacking (single pawn check for both)
    if (is_pawn(piece)) {
        int8_t delta = to - from;
        if (move & MOVE_WHITE_MASK) {
            if (delta == 16) {
                game->b_en_passant_square = from + 8;
            }
            if (game->w_en_passant_square == to) {
                square_clear(game, to - 8);         // captured black pawn
            }
        }
        else {
            if (delta == -16) {
                game->w_en_passant_square = from - 8;
            }
            if (game->b_en_passant_square == to) {
                square_clear(game, to + 8);         // captured white pawn
            }
        }
    }

    // castling relocates the rook; the king itself is moved by the block below
    if (is_king(piece) && (move & MOVE_KSC_FLAG_MASK)) {
        rook = game->board[from + 3];
        square_clear(game, from + 3);
        square_set(game, from + 1, rook);
    }
    else if (is_king(piece) && (move & MOVE_QSC_FLAG_MASK)) {
        rook = game->board[from - 4];
        square_clear(game, from - 4);
        square_set(game, from - 1, rook);
    }

    // castling rights depend on the moving piece still sitting on 'from'
    modify_castling_rights(game, move);

    // pawn promotion: choose the piece that lands on 'to'
    if (move & MOVE_NP_FLAG_MASK) {
        landing = piece_color | KNIGHT;
    }
    else if (move & MOVE_BP_FLAG_MASK) {
        landing = piece_color | BISHOP;
    }
    else if (move & MOVE_RP_FLAG_MASK) {
        landing = piece_color | ROOK;
    }
    else if (move & MOVE_QP_FLAG_MASK) {
        landing = piece_color | QUEEN;
    }

    // track the king's square for fast check tests
    if (is_king(piece)) {
        if (move & MOVE_WHITE_MASK) {
            game->king_square_w = to;
        }
        else {
            game->king_square_b = to;
        }
    }

    // apply the move: remove any captured piece, lift the mover, drop it on 'to'
    square_clear(game, to);
    square_clear(game, from);
    square_set(game, to, landing);
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

    return parse_fen(buffer);
}


Game *parse_fen(const char *buffer) {
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
        en_passant_rank -= '1';

        uint8_t en_passant_target_square = en_passant_file + (en_passant_rank * 8);

        // the side to move is the one that can capture en passant, so the
        // target square goes in that color's en passant slot (mirrors move_piece)
        if (!game->move) {
            game->w_en_passant_square = en_passant_target_square;
        }
        else {
            game->b_en_passant_square = en_passant_target_square;
        }
        index++;
    }

    index++;


    uint16_t half_moves_count = buffer[index];
    index++;
    uint16_t full_moves_count = buffer[index];

    // derive the bitboards from the mailbox we just parsed
    rebuild_bitboards(game);

    return game;

}


// Serializes the game position into a FEN string in 'buffer'. Only the fields
// the engine tracks are emitted; halfmove/fullmove counters are written as
// "0 1". Used to report positions (e.g. from --crosscheck) for the python-chess
// oracle in tools/.
void game_to_fen(Game *game, char *buffer) {
    int idx;        // write cursor into buffer
    int rank;       // rank being written (8 down to 1)
    int file;       // file within the rank
    int empty;      // run length of empty squares
    uint8_t piece;  // piece on the current square
    char c;         // its FEN letter
    int any;        // whether any castling right is present
    uint8_t ep;     // en passant target square for the side to move

    idx = 0;

    // piece placement, rank 8 first
    for (rank = 7; rank >= 0; rank--) {
        empty = 0;
        for (file = 0; file < 8; file++) {
            piece = game->board[rank * 8 + file];
            if (piece == NONE) {
                empty++;
                continue;
            }
            if (empty) {
                buffer[idx++] = '0' + empty;
                empty = 0;
            }
            switch (PIECE_MASK(piece)) {
                case PAWN:   c = 'p'; break;
                case KNIGHT: c = 'n'; break;
                case BISHOP: c = 'b'; break;
                case ROOK:   c = 'r'; break;
                case QUEEN:  c = 'q'; break;
                default:     c = 'k'; break;
            }
            if (piece & WHITE) {
                c = c - 'a' + 'A';      // white pieces are uppercase
            }
            buffer[idx++] = c;
        }
        if (empty) {
            buffer[idx++] = '0' + empty;
        }
        if (rank) {
            buffer[idx++] = '/';
        }
    }

    // side to move
    buffer[idx++] = ' ';
    buffer[idx++] = game->move ? 'b' : 'w';

    // castling rights
    buffer[idx++] = ' ';
    any = 0;
    if (game->castle_kingside_w)  { buffer[idx++] = 'K'; any = 1; }
    if (game->castle_queenside_w) { buffer[idx++] = 'Q'; any = 1; }
    if (game->castle_kingside_b)  { buffer[idx++] = 'k'; any = 1; }
    if (game->castle_queenside_b) { buffer[idx++] = 'q'; any = 1; }
    if (!any) {
        buffer[idx++] = '-';
    }

    // en passant target square (the side to move's capture square)
    buffer[idx++] = ' ';
    ep = game->move ? game->b_en_passant_square : game->w_en_passant_square;
    if (ep <= 63) {
        buffer[idx++] = 'a' + (ep % 8);
        buffer[idx++] = '1' + (ep / 8);
    }
    else {
        buffer[idx++] = '-';
    }

    // halfmove / fullmove counters (not tracked; placeholders)
    buffer[idx++] = ' ';
    buffer[idx++] = '0';
    buffer[idx++] = ' ';
    buffer[idx++] = '1';
    buffer[idx] = '\0';
} /* game_to_fen */
