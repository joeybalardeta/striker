#include "piece.h"
#include <stdint.h>

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
	if ((piece & 0x7) == 1) {
		return 1;
	}
	return 0;
}

