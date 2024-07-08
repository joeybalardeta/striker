#ifndef MOVELISTH
#define MOVELISTH

#include <stdint.h>

typedef struct _MoveList MoveList;
typedef struct _MoveListEntry MoveListEntry;

struct _MoveList {
    uint32_t length;
    MoveListEntry *first;
};

struct _MoveListEntry {
    MoveListEntry *next;
    uint16_t move;
};

// movelist functions
MoveList *create_movelist();
void delete_movelist(MoveList *movelist);

MoveListEntry *get_movelistentry(MoveList *movelist, uint32_t index);
void add_movelistentry(MoveList *movelist, MoveListEntry *movelistentry);
void remove_movelistentry(MoveList *movelist, uint32_t index);
void clear_movelist(MoveList *movelist);

// movelistentry functions
MoveListEntry *create_movelistentry(uint16_t move);
void delete_movelistentry(MoveListEntry *movelistentry);


#endif
