#include <stdlib.h>
#include <stdint.h>
#include "movelist.h"

// movelist functions
MoveList *create_movelist() {
    MoveList *movelist = (MoveList *) malloc(sizeof(MoveList));

    movelist->length = 0;
    movelist->first = NULL;

    return movelist;
}


void delete_movelist(MoveList *movelist) {
    if (!movelist) {
        return;
    }

    clear_movelist(movelist);

    free(movelist);
}


MoveListEntry *get_movelistentry(MoveList *movelist, uint32_t index) {
    uint32_t length = movelist->length;

    if (index + 1 >= length) {
        return NULL;
    }

    MoveListEntry *movelistentry = movelist->first;

    for (uint32_t i = 0; i < index; i++) {
        movelistentry = movelistentry->next;
    }

    return movelistentry;
}


void add_movelistentry(MoveList *movelist, MoveListEntry *movelistentry) {
    if (!movelistentry) {
        return;
    }

    if (movelist->length == 0) {
        movelist->first = movelistentry;
    }
    else {
        MoveListEntry *last = get_movelistentry(movelist, movelist->length - 1);
        last->next = movelistentry;
    }

    movelist->length++;
}


void remove_movelistentry(MoveList *movelist, uint32_t index) {
    if (movelist->length == 0) {
        return;
    }

    MoveListEntry *prev = get_movelistentry(movelist, index - 1);
    MoveListEntry *to_remove = get_movelistentry(movelist, index);

    if (prev) {
        prev->next = NULL;
    }
    else {
        movelist->first = movelist->first->next;
    }

    movelist->length--;

    delete_movelistentry(to_remove);
}


void clear_movelist(MoveList *movelist) {
    while (movelist->first) {
        remove_movelistentry(movelist, 0);
    }
}

// movelistentry functions
MoveListEntry *create_movelistentry(uint16_t move) {
    MoveListEntry *movelistentry = (MoveListEntry *) malloc(sizeof(MoveListEntry));

    movelistentry->move = move;
    movelistentry->next = NULL;

    return movelistentry;
}


void delete_movelistentry(MoveListEntry *movelistentry) {
    if (!movelistentry) {
        return;
    }
    free(movelistentry);
}
