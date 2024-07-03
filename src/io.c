#include <stdio.h>
#include <stdint.h>
#include "io.h"
#include "utils.h"

uint16_t get_user_move() {
    char from_rank, from_file, to_rank, to_file;
    
    char buffer[100];

    scanf("%s", buffer);
    sscanf(buffer, "%c%c,%c%c", &from_rank, &from_file, &to_rank, &to_file);

    from_rank -= 'a';
    from_file -= '1';
    to_rank -= 'a';
    to_file -= '1';

    uint8_t from = from_rank * 8 + from_file;
    uint8_t to = to_rank * 8 + to_file;

    uint16_t move = from + (to << 8);
    return move;
}


uint32_t get_user_option() {
    uint32_t input;
    scanf("%u", &input);
    return input;
}
