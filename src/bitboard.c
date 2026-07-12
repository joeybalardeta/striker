#include <stdio.h>
#include <stdint.h>
#include "bitboard.h"

/* Bitboard attack toolkit.
 *
 * Non-sliding pieces (knight, king, pawn) use simple precomputed per-square
 * tables. Sliding pieces (rook, bishop, queen) use "magic bitboards": for each
 * square we mask off the relevant occupancy bits, multiply by a magic constant
 * to hash that occupancy into a dense index, and look the attack set up in a
 * per-square table. The magic constants are found by trial at startup so there
 * are no large hardcoded tables to maintain.
 */

/* public attack tables */
Bitboard knight_attacks[64];
Bitboard king_attacks[64];
Bitboard pawn_attacks[2][64];

/* Per-square magic entry for one sliding piece kind. 'attacks' points into the
 * shared backing table below; index = ((occ & mask) * magic) >> shift.
 */
typedef struct {
    Bitboard mask;
    Bitboard magic;
    Bitboard *attacks;
    int shift;
} Magic;

static Magic rook_tbl[64];
static Magic bishop_tbl[64];

/* Backing storage for the magic lookups. The standard dense sizes are 102400
 * entries for rooks and 5248 for bishops across all 64 squares.
 */
static Bitboard rook_attack_data[102400];
static Bitboard bishop_attack_data[5248];

/* sliding directions as (rank delta, file delta) pairs */
static const int rook_dirs[4][2]   = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
static const int bishop_dirs[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};


/* Computes a sliding attack set the slow way by walking each direction from
 * 'sq' until the board edge or an occupied square (which is included). Used
 * only during initialization to build the magic tables.
 */
static Bitboard sliding_attacks(int sq, Bitboard occ, const int dirs[4][2]) {
    Bitboard attacks;
    int r, f;       /* source rank/file */
    int i;          /* direction index */
    int rr, ff;     /* running rank/file while walking a direction */

    attacks = 0;
    r = sq / 8;
    f = sq % 8;

    /* walk out along each of the four directions */
    for (i = 0; i < 4; i++) {
        rr = r + dirs[i][0];
        ff = f + dirs[i][1];
        while (rr >= 0 && rr <= 7 && ff >= 0 && ff <= 7) {
            attacks |= BB_SQ(rr * 8 + ff);
            if (occ & BB_SQ(rr * 8 + ff)) {
                break;      /* blocked: include the blocker, then stop */
            }
            rr += dirs[i][0];
            ff += dirs[i][1];
        }
    }

    return attacks;
} /* sliding_attacks */


/* Builds the relevant-occupancy mask for a sliding piece on 'sq': the squares
 * whose occupancy can affect the attack set, excluding the board edges (a
 * blocker on the far edge never changes what lies beyond it).
 */
static Bitboard sliding_mask(int sq, const int dirs[4][2]) {
    Bitboard mask;
    int r, f;
    int i;
    int rr, ff;

    mask = 0;
    r = sq / 8;
    f = sq % 8;

    /* walk each direction, including a square only when the step beyond it is
     * still on the board. this drops the final (edge) square in that direction,
     * which is exactly the relevant-occupancy mask for both rooks and bishops.
     */
    for (i = 0; i < 4; i++) {
        rr = r + dirs[i][0];
        ff = f + dirs[i][1];
        while (rr >= 0 && rr <= 7 && ff >= 0 && ff <= 7) {
            int nr = rr + dirs[i][0];   /* next square along this direction */
            int nf = ff + dirs[i][1];
            if (nr < 0 || nr > 7 || nf < 0 || nf > 7) {
                break;                  /* (rr,ff) is the edge square: exclude it */
            }
            mask |= BB_SQ(rr * 8 + ff);
            rr += dirs[i][0];
            ff += dirs[i][1];
        }
    }

    return mask;
} /* sliding_mask */


/* Maps the 'index'-th subset of the set bits in 'mask' to a bitboard. Used to
 * enumerate every possible occupancy of a magic mask (bits = popcount(mask),
 * index in 0 .. 2^bits - 1).
 */
static Bitboard index_to_occupancy(int index, int bits, Bitboard mask) {
    Bitboard occ;
    int i;
    int sq;

    occ = 0;

    /* pull out mask bits one at a time; each bit of 'index' selects one */
    for (i = 0; i < bits; i++) {
        sq = bb_pop_lsb(&mask);
        if (index & (1 << i)) {
            occ |= BB_SQ(sq);
        }
    }

    return occ;
} /* index_to_occupancy */


/* Simple, reproducible xorshift64 RNG so magic search is deterministic. */
static uint64_t rng_state = 0x9E3779B97F4A7C15ULL;

static uint64_t rng_next(void) {
    rng_state ^= rng_state >> 12;
    rng_state ^= rng_state << 25;
    rng_state ^= rng_state >> 27;
    return rng_state * 0x2545F4914F6CDD1DULL;
} /* rng_next */


/* Returns a candidate magic: ANDing three random values yields a sparse number,
 * which is what tends to make a working magic multiplier.
 */
static Bitboard random_magic(void) {
    return rng_next() & rng_next() & rng_next();
} /* random_magic */


/* Finds a magic for one square and fills its dense attack table. 'dirs' selects
 * rook vs bishop; 'data' is the shared backing store and 'offset' where this
 * square's block begins.
 */
static void init_magic_square(int sq, const int dirs[4][2], Magic *entry,
                              Bitboard *data, int offset) {
    Bitboard mask;
    int bits;
    int size;
    Bitboard occ[4096];         /* every occupancy subset for this square */
    Bitboard ref[4096];         /* the true attacks for each subset */
    int i;                      /* subset index */
    Bitboard magic;             /* candidate magic being tested */
    Bitboard *used;             /* this square's slice of 'data' */
    int fail;                   /* set if a candidate collides */
    int idx;                    /* hashed index for a subset */

    mask = sliding_mask(sq, dirs);
    bits = bb_popcount(mask);
    size = 1 << bits;

    entry->mask = mask;
    entry->shift = 64 - bits;
    entry->attacks = data + offset;
    used = entry->attacks;

    /* precompute every occupancy subset and its true attack set */
    for (i = 0; i < size; i++) {
        occ[i] = index_to_occupancy(i, bits, mask);
        ref[i] = sliding_attacks(sq, occ[i], dirs);
    }

    /* trial random magics until one hashes all subsets without collision */
    while (1) {
        magic = random_magic();

        /* cheap reject: the top of mask*magic must spread enough bits */
        if (bb_popcount((mask * magic) & 0xFF00000000000000ULL) < 6) {
            continue;
        }

        for (i = 0; i < size; i++) {
            used[i] = 0;
        }

        fail = 0;
        for (i = 0; i < size && !fail; i++) {
            idx = (int) ((occ[i] * magic) >> entry->shift);
            if (used[idx] == 0) {
                used[idx] = ref[i];
            }
            else if (used[idx] != ref[i]) {
                fail = 1;       /* two occupancies collide with different attacks */
            }
        }

        if (!fail) {
            entry->magic = magic;
            return;
        }
    }
} /* init_magic_square */


/* Fills the knight and king per-square attack tables. */
static void init_leaper_attacks(void) {
    static const int knight_off[8][2] = {
        {2, 1}, {2, -1}, {-2, 1}, {-2, -1},
        {1, 2}, {1, -2}, {-1, 2}, {-1, -2}
    };
    static const int king_off[8][2] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1},
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
    };
    int sq;
    int r, f;
    int i;
    int rr, ff;

    for (sq = 0; sq < 64; sq++) {
        r = sq / 8;
        f = sq % 8;
        knight_attacks[sq] = 0;
        king_attacks[sq] = 0;

        for (i = 0; i < 8; i++) {
            /* knight jumps */
            rr = r + knight_off[i][0];
            ff = f + knight_off[i][1];
            if (rr >= 0 && rr <= 7 && ff >= 0 && ff <= 7) {
                knight_attacks[sq] |= BB_SQ(rr * 8 + ff);
            }
            /* king steps */
            rr = r + king_off[i][0];
            ff = f + king_off[i][1];
            if (rr >= 0 && rr <= 7 && ff >= 0 && ff <= 7) {
                king_attacks[sq] |= BB_SQ(rr * 8 + ff);
            }
        }
    }
} /* init_leaper_attacks */


/* Fills the pawn capture tables for both colors. White captures up the board
 * (+rank), black captures down.
 */
static void init_pawn_attacks(void) {
    int sq;
    int r, f;

    for (sq = 0; sq < 64; sq++) {
        r = sq / 8;
        f = sq % 8;
        pawn_attacks[BB_WHITE][sq] = 0;
        pawn_attacks[BB_BLACK][sq] = 0;

        /* white: one rank up, one file to each side */
        if (r < 7 && f > 0) {
            pawn_attacks[BB_WHITE][sq] |= BB_SQ((r + 1) * 8 + (f - 1));
        }
        if (r < 7 && f < 7) {
            pawn_attacks[BB_WHITE][sq] |= BB_SQ((r + 1) * 8 + (f + 1));
        }
        /* black: one rank down, one file to each side */
        if (r > 0 && f > 0) {
            pawn_attacks[BB_BLACK][sq] |= BB_SQ((r - 1) * 8 + (f - 1));
        }
        if (r > 0 && f < 7) {
            pawn_attacks[BB_BLACK][sq] |= BB_SQ((r - 1) * 8 + (f + 1));
        }
    }
} /* init_pawn_attacks */


void init_attack_tables(void) {
    int sq;
    int rook_off;       /* running offset into rook_attack_data */
    int bishop_off;     /* running offset into bishop_attack_data */

    init_leaper_attacks();
    init_pawn_attacks();

    /* lay each square's magic table end to end in the shared backing arrays */
    rook_off = 0;
    bishop_off = 0;
    for (sq = 0; sq < 64; sq++) {
        init_magic_square(sq, rook_dirs, &rook_tbl[sq], rook_attack_data, rook_off);
        rook_off += 1 << bb_popcount(rook_tbl[sq].mask);

        init_magic_square(sq, bishop_dirs, &bishop_tbl[sq], bishop_attack_data, bishop_off);
        bishop_off += 1 << bb_popcount(bishop_tbl[sq].mask);
    }
} /* init_attack_tables */


Bitboard rook_attacks(int sq, Bitboard occ) {
    Magic *m = &rook_tbl[sq];
    return m->attacks[((occ & m->mask) * m->magic) >> m->shift];
} /* rook_attacks */


Bitboard bishop_attacks(int sq, Bitboard occ) {
    Magic *m = &bishop_tbl[sq];
    return m->attacks[((occ & m->mask) * m->magic) >> m->shift];
} /* bishop_attacks */


Bitboard queen_attacks(int sq, Bitboard occ) {
    return rook_attacks(sq, occ) | bishop_attacks(sq, occ);
} /* queen_attacks */


void bb_print(Bitboard b) {
    int r, f;

    for (r = 7; r >= 0; r--) {
        for (f = 0; f < 8; f++) {
            printf("%c ", (b & BB_SQ(r * 8 + f)) ? 'X' : '.');
        }
        printf("\n");
    }
    printf("\n");
} /* bb_print */
