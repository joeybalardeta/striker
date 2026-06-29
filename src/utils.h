#ifndef UTILSH
#define UTILSH

#include <stdint.h>

uint8_t str_to_square(char *str);
void print_square(uint8_t square);
void print_move(uint32_t move);
uint64_t get_time_us();
uint64_t get_time_ns();
void print_elapsed_time(uint64_t elapsed_ns);

#endif
