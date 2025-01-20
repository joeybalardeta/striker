#ifndef IOH
#define IOH

#include <stdint.h>

uint16_t get_user_move();
uint32_t get_user_option();

uint8_t load_fen(const char *filepath, char *buffer);

#endif
