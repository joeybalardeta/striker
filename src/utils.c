#include <stdio.h>
#include <stdint.h>
#include "utils.h"


uint8_t str_to_square(char *str) {
    char file = str[0] - 'a';
    char rank = str[1] - '1';

    uint8_t square = file + (rank * 8);

    return square;
}


void print_square(uint8_t square) {
    char file = 'a' + (square % 8);
    char rank = '1' + (square / 8);

    printf("%c%c", file, rank);
}


void print_move(uint32_t move) {
    uint8_t from = move & 0xFF;
    uint8_t to = (move >> 8) & 0xFF;
    // uint16_t flags = (move >> 16) & 0xFFFF;

    print_square(from);
    printf(", ");
    print_square(to);
}
