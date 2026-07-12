#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "movelist.h"
#include "utils.h"

// movelist functions
MoveList *create_movelist() {
    MoveList *movelist = (MoveList *) malloc(sizeof(MoveList));

    movelist->length = 0;

    return movelist;
} /* create_movelist */


void delete_movelist(MoveList *movelist) {
    if (!movelist) {
        return;
    }

    free(movelist);
} /* delete_movelist */


void print_movelist(MoveList *movelist) {
    uint32_t i;     // index into the move array

    printf("MoveList (@ %p)\n", (void *) movelist);
    for (i = 0; i < movelist->length; i++) {
        printf("    ");
        print_move(movelist->moves[i]);
        printf("\n");
    }
} /* print_movelist */
