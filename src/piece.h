#ifndef PIECEH
#define PIECEH

#include <stdint.h>

typedef enum {
    // first 3 bits
    NONE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
    
    // next 2 bits
    WHITE = 8,
    BLACK = 16
} Piece;

void get_piece_str(uint8_t piece, char *buffer);
uint8_t is_piece(uint8_t piece);
uint8_t is_white(uint8_t piece);
uint8_t is_pawn(uint8_t piece);
uint8_t is_knight(uint8_t piece);
uint8_t is_bishop(uint8_t piece);
uint8_t is_rook(uint8_t piece);
uint8_t is_queen(uint8_t piece);
uint8_t is_king(uint8_t piece);


#endif
