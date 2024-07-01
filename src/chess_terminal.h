#ifndef CHESSTERMINALH
#define CHESSTERMINALH

#include <stdint.h>

#define EXIT_INPUT 9

void chess_terminal();
uint32_t get_input();
void execute_input(uint32_t input);
void print_options();

#endif
