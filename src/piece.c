#include <stdint.h>
#include "piece.h"


void get_piece_str(uint8_t piece, char *buffer) {
    // function assumes an input buffer of at least length 3

    buffer[2] = '\0';

    if (!is_piece(piece)) {
        buffer[0] = ' ';
        buffer[1] = ' ';
        return;
    }

    if (is_white(piece)) {
        buffer[0] = 'W';
    }
    else {
        buffer[0] = 'B';
    }
    
    char piece_str = '0';
    if (is_pawn(piece)) {
        piece_str = 'P';
    }
    else if (is_knight(piece)) {
        piece_str = 'N';
    }
    else if (is_bishop(piece)) {
        piece_str = 'B';
    }
    else if (is_rook(piece)) {
        piece_str = 'R';
    }
    else if (is_queen(piece)) {
        piece_str = 'Q';
    }
    else {
        piece_str = 'K';
    }

    buffer[1] = piece_str;
}


uint8_t is_piece(uint8_t piece) {
    if (piece) {
        return 1;
    }
    return 0;
}


uint8_t is_white(uint8_t piece) {
    if (piece & 0x8) {
        return 1;
    }
    return 0;
}


uint8_t is_pawn(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == PAWN) {
        return 1;
    }
    return 0;
}


uint8_t is_knight(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == KNIGHT) {
        return 1;
    }
    return 0;
}


uint8_t is_bishop(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == BISHOP) {
        return 1;
    }
    return 0;
}


uint8_t is_rook(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == ROOK) {
        return 1;
    }
    return 0;
}


uint8_t is_queen(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == QUEEN) {
        return 1;
    }
    return 0;
}


uint8_t is_king(uint8_t piece) {
    // isolate first 3 bits
    if ((piece & 0x7) == KING) {
        return 1;
    }
    return 0;
}

