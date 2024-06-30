#include <stdio.h>
#include "piece.h"

#define VERSION "0.0.1"

int main(int argc, char **argv) {
	printf("Striker Chess Engine\n");
	printf("Version: %s\n\n", VERSION);


	printf("Piece enum testing:\n");
	Piece piece = PAWN | WHITE;

	printf("is_piece(): %d\n", is_piece(piece));
	printf("is_white(): %d\n", is_white(piece));
	printf("is_pawn(): %d\n", is_pawn(piece));

	return 0;
}
