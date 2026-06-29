#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "movelist.h"
#include "utils.h"

// movelist functions
MoveList *create_movelist() {
    MoveList *movelist = (MoveList *) malloc(sizeof(MoveList));

    movelist->length = 0;
    movelist->first = NULL;
    movelist->last = NULL;

    return movelist;
}


void delete_movelist(MoveList *movelist) {
    if (!movelist) {
        return;
    }

    // entries are inline, so a single free releases the whole list
    free(movelist);
}


MoveListEntry *get_movelistentry(MoveList *movelist, uint32_t index) {
    if (index >= movelist->length) {
        return NULL;
    }

    // entries are contiguous: direct O(1) indexing
    return &movelist->entries[index];
}


void add_move(MoveList *movelist, uint32_t move) {
    if (movelist->length >= MOVELIST_CAPACITY) {
        return;  // cannot happen in legal chess; guards against overflow
    }

    MoveListEntry *entry = &movelist->entries[movelist->length];
    entry->move = move;
    entry->next = NULL;

    if (movelist->length == 0) {
        movelist->first = entry;
    }
    else {
        movelist->last->next = entry;
    }
    movelist->last = entry;

    movelist->length++;
}


void clear_movelist(MoveList *movelist) {
    movelist->first = NULL;
    movelist->last = NULL;
    movelist->length = 0;
}


void print_movelist(MoveList *movelist) {
    printf("MoveList (@ %p)\n", movelist);
    MoveListEntry *mle = movelist->first;
    for (int i = 0; i < movelist->length; i++) {
        printf("    ");
        print_move(mle->move);
        printf("\n");
        mle = mle->next;
    }
}
