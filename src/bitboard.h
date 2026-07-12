#ifndef BITBOARDH
#define BITBOARDH

#include <stdint.h>

/* A Bitboard is a 64-bit set with one bit per square, bit i == square i,
 * using the same square layout as the rest of the engine: sq = file + rank*8,
 * a1 = 0 ... h8 = 63.
 */
typedef uint64_t Bitboard;

/* colors used to index the per-color tables; mirror the WHITE/BLACK ordering */
#define BB_WHITE 0
#define BB_BLACK 1

/* single-square bit mask */
#define BB_SQ(sq) (((Bitboard) 1) << (sq))

/* Precomputed attack sets for the non-sliding pieces. pawn_attacks is indexed
 * [color][square] and holds the two diagonal capture squares for a pawn of that
 * color standing on that square.
 */
extern Bitboard knight_attacks[64];
extern Bitboard king_attacks[64];
extern Bitboard pawn_attacks[2][64];

/* Returns the number of set bits (pieces) in a bitboard. */
static inline int bb_popcount(Bitboard b) {
    return __builtin_popcountll(b);
} /* bb_popcount */

/* Returns the index of the least significant set bit. Undefined if b == 0. */
static inline int bb_lsb(Bitboard b) {
    return __builtin_ctzll(b);
} /* bb_lsb */

/* Returns the index of the least significant set bit and clears it in place. */
static inline int bb_pop_lsb(Bitboard *b) {
    int sq = __builtin_ctzll(*b);
    *b &= *b - 1;
    return sq;
} /* bb_pop_lsb */

/* Builds all attack tables and finds the sliding-piece magics. Must be called
 * once at program start before any move generation.
 */
void init_attack_tables(void);

/* Sliding-piece attack lookups: the set of squares a rook/bishop/queen on 'sq'
 * attacks given the full-board occupancy 'occ' (attacks stop at, and include,
 * the first occupied square in each direction).
 */
Bitboard rook_attacks(int sq, Bitboard occ);
Bitboard bishop_attacks(int sq, Bitboard occ);
Bitboard queen_attacks(int sq, Bitboard occ);

/* Debug helper: prints a bitboard as an 8x8 grid (rank 8 at top). */
void bb_print(Bitboard b);

#endif
