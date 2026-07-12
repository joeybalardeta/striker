#ifndef MOVELISTH
#define MOVELISTH

#include <stdint.h>

// upper bound on legal moves in any chess position is 218; round up for safety
#define MOVELIST_CAPACITY 256

// A move list is a flat, fixed-capacity array of encoded moves. Generation is
// the hottest path in the engine, so moves are stored contiguously (good cache
// behaviour, O(1) append and index) rather than in a linked structure.
typedef struct {
    uint32_t length;
    uint32_t moves[MOVELIST_CAPACITY];
} MoveList;

// heap-allocated list helpers (used where a list outlives a stack frame)
MoveList *create_movelist();
void delete_movelist(MoveList *movelist);

// Appends a move. Inline and bounds-check-free: legal chess never exceeds the
// capacity, and callers pass stack lists in the generation hot path.
static inline void add_move(MoveList *movelist, uint32_t move) {
    movelist->moves[movelist->length++] = move;
}

// Empties the list for reuse.
static inline void clear_movelist(MoveList *movelist) {
    movelist->length = 0;
}

// utils functions
void print_movelist(MoveList *movelist);

#endif
