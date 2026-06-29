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


uint64_t get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return ((uint64_t) tv.tv_sec * 1000000ULL) + (uint64_t) tv.tv_usec;
}


// gets thread specific time
uint64_t get_time_ns() {
    struct timespec ts;

    if (clock_gettime(CLOCK_TYPE, &ts) == 0) {
        return ((uint64_t) ts.tv_sec * 1000000000ULL) + (uint64_t) ts.tv_nsec;
    } else {
        return 0;
    }
}


// prints an elapsed nanosecond duration using the largest unit that keeps
// the value >= 1 (microseconds -> milliseconds -> seconds)
void print_elapsed_time(uint64_t elapsed_ns) {
    if (elapsed_ns < 1000000ULL) {              // under 1 ms
        printf("%.2fus", elapsed_ns / 1000.0);
    }
    else if (elapsed_ns < 1000000000ULL) {      // under 1 s
        printf("%.2fms", elapsed_ns / 1000000.0);
    }
    else {
        printf("%.2fs", elapsed_ns / 1000000000.0);
    }
}
