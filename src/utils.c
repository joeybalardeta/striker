#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "utils.h"


// other define statements
#define CLOCK_TYPE CLOCK_REALTIME
// #define CLOCK_TYPE CLOCK_THREAD_CPUTIME_ID


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


uint32_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 1000000) + tv.tv_usec;
}


// gets thread specific time
uint32_t get_time_ns() {
    struct timespec ts;

    if (clock_gettime(CLOCK_TYPE, &ts) == 0) {
        return (ts.tv_sec * 1000000000) + ts.tv_nsec;
    } else {
        return 0;
    }
}
