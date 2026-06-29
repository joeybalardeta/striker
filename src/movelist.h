#ifndef MOVELISTH
#define MOVELISTH

#include <stdint.h>

// upper bound on legal moves in any chess position is 218; round up for safety
#define MOVELIST_CAPACITY 256

typedef struct _MoveList MoveList;
typedef struct _MoveListEntry MoveListEntry;

struct _MoveListEntry {
    MoveListEntry *next;
    uint32_t move;
};

struct _MoveList {
    uint32_t length;
    MoveListEntry *first;
    MoveListEntry *last;
    // entries live inline so the whole list is a single allocation and the
    // 'next' links stay valid (no per-move malloc/free churn)
    MoveListEntry entries[MOVELIST_CAPACITY];
};

// movelist functions
MoveList *create_movelist();
void delete_movelist(MoveList *movelist);

MoveListEntry *get_movelistentry(MoveList *movelist, uint32_t index);
void add_move(MoveList *movelist, uint32_t move);
void clear_movelist(MoveList *movelist);

// utils functions
void print_movelist(MoveList *movelist);

#endif
