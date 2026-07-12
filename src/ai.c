#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ai.h"
#include "game.h"
#include "player.h"
#include "piece.h"
#include "movegen.h"
#include "movelist.h"
#include "utils.h"

// the archived mailbox generator, kept as a reference oracle for --crosscheck
void generate_moves_mailbox(Game *game, MoveList *possible_moves);

// debug define macros (enables compilation of printf statements)
// #define DEBUG_AI
// #define DEBUG_PERFT

uint32_t get_computer_move(Game *game, uint8_t player) {
    #ifdef DEBUG_AI
    printf("Calculating move for ");
    printf(player == PLAYERW ? "White" : "Black");
    printf("...\n");
    #endif

    MoveList *possible_moves = get_possible_moves(game);

    srand(time(NULL)); 

    uint32_t move = possible_moves->moves[rand() % possible_moves->length];

    delete_movelist(possible_moves);

    return move;
}


void print_possible_moves(Game *game) {
    uint8_t player = !game->move ? PLAYERW : PLAYERB;
    printf("Getting possible moves for ");
    printf(player == PLAYERW ? "White" : "Black");
    printf("...\n");
    
    MoveList *possible_moves = get_possible_moves(game);
    printf("%d possible moves found.\n\n", possible_moves->length);

    print_movelist(possible_moves);

    delete_movelist(possible_moves);
}


// known-good perft results for standard test positions, verified depth by
// depth (1..MAX_VALIDATE_DEPTH). these exercise castling, en passant,
// promotions, checks and pins. counts are from the Chess Programming Wiki
// (standard positions) and python-chess (the rest).
#define MAX_VALIDATE_DEPTH 4

typedef struct {
    const char *name;
    const char *fen;
    uint32_t expected[MAX_VALIDATE_DEPTH];  // expected node count at depths 1..N
} PerftCase;

int run_validation() {
    static const PerftCase cases[] = {
        {"startpos",          "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                              {20, 400, 8902, 197281}},
        {"kiwipete",          "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
                              {48, 2039, 97862, 4085603}},
        {"position 3",        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
                              {14, 191, 2812, 43238}},
        {"position 4",        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
                              {6, 264, 9467, 422333}},
        {"position 5",        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
                              {44, 1486, 62379, 2103487}},
        {"position 6",        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
                              {46, 2079, 89890, 3894594}},
        {"position 4 mirror", "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1",
                              {6, 264, 9467, 422333}},
        {"en passant",        "8/8/8/2k5/2pP4/8/B7/4K3 b - d3 0 3",
                              {8, 72, 492, 5380}},
        {"ep pin / checks",   "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 b - - 0 1",
                              {15, 205, 3047, 45673}},
        {"promotions",        "n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1",
                              {24, 496, 9483, 182838}},
    };

    int n = sizeof(cases) / sizeof(cases[0]);
    int failures = 0;

    printf("Running move generation validation (%d positions, depth 1-%d):\n\n",
           n, MAX_VALIDATE_DEPTH);

    for (int i = 0; i < n; i++) {
        printf("%-18s %s\n", cases[i].name, cases[i].fen);

        for (int d = 1; d <= MAX_VALIDATE_DEPTH; d++) {
            Game *game = parse_fen(cases[i].fen);

            uint64_t start_time = get_time_ns();
            uint32_t result = perft(game, d);
            uint64_t end_time = get_time_ns();

            delete_game(game);

            uint32_t expected = cases[i].expected[d - 1];
            uint8_t pass = (result == expected);
            if (!pass) {
                failures++;
            }

            printf("  [%s] depth %d: %9u (expected %9u) | ",
                   pass ? "PASS" : "FAIL", d, result, expected);
            print_elapsed_time(end_time - start_time);
            printf("\n");
        }
        printf("\n");
    }

    int total = n * MAX_VALIDATE_DEPTH;
    printf("Validation: %d/%d checks passed.\n", total - failures, total);

    return failures;
}


// fixed perft suite (~16M leaf nodes) for measuring move-gen throughput.
void run_benchmark() {
    static const struct {
        const char *fen;
        int8_t depth;
    } suite[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5},
        {"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4},
        {"8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5},
        {"r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4},
        {"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4},
        {"r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4},
    };

    int n = sizeof(suite) / sizeof(suite[0]);
    uint64_t total_nodes = 0;
    uint64_t total_ns = 0;

    printf("Running benchmark (%d positions):\n\n", n);

    for (int i = 0; i < n; i++) {
        Game *game = parse_fen(suite[i].fen);

        uint64_t start_time = get_time_ns();
        uint32_t nodes = perft(game, suite[i].depth);
        uint64_t end_time = get_time_ns();

        delete_game(game);

        uint64_t elapsed = end_time - start_time;
        total_nodes += nodes;
        total_ns += elapsed;

        printf("  depth %d: %9u nodes | ", suite[i].depth, nodes);
        print_elapsed_time(elapsed);
        printf("\n");
    }

    double seconds = total_ns / 1000000000.0;
    double mnps = (total_nodes / seconds) / 1000000.0;

    printf("\nTotal: %llu nodes in ", (unsigned long long) total_nodes);
    print_elapsed_time(total_ns);
    printf(" | %.2f Mnps\n", mnps);
}


void run_perft_test(Game *game, uint32_t max_depth) {
    printf("Running PERFT test:\n\n");

    for (uint32_t i = 1; i <= max_depth; i++) {
        uint64_t start_time = get_time_ns();
        uint32_t moves = perft(game, i);
        uint64_t end_time = get_time_ns();

        uint64_t elapsed_ns = end_time - start_time;

        printf("Depth %u: %9u moves found | ", i, moves);
        print_elapsed_time(elapsed_ns);
        printf("\n");
    }
    printf("\nPERFT test done.\n");
}


uint32_t perft(Game *game, int8_t depth) {
    static uint8_t max_depth = 0;

    if (depth > max_depth) {
        max_depth = depth;
    }

    if (depth <= 0) {
        return 1;
    }

    // stack-allocated list avoids a heap allocation per perft node
    MoveList possible_moves;
    generate_moves(game, &possible_moves);

    if (depth == 1) {
        return possible_moves.length;
    }

    uint32_t total_moves = 0;

    for (uint32_t i = 0; i < possible_moves.length; i++) {
        uint32_t move = possible_moves.moves[i];
        // stack copy avoids a malloc/free per node in the perft tree
        Game clone;
        memcpy(&clone, game, sizeof(Game));
        move_piece(&clone, move);
        change_turn(&clone);
        uint32_t moves = perft(&clone, depth - 1);

        #ifdef DEBUG_PERFT
        if (depth == max_depth) {
            // perft divide: "<from><to>: <count>" e.g. e2e4: 9329
            print_square(move & 0xFF);
            print_square((move >> 8) & 0xFF);
            printf(": %u\n", moves);
        }
        #endif

        total_moves += moves;
    }

    return total_moves;
}


// orders two moves so a move list can be compared as a set
static int move_cmp(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *) a;
    uint32_t y = *(const uint32_t *) b;
    return (x > y) - (x < y);
} /* move_cmp */


// copies a move list's moves into 'out' and sorts them, returning the count
static int collect_sorted(MoveList *list, uint32_t *out) {
    int n = (int) list->length;
    int i;

    for (i = 0; i < n; i++) {
        out[i] = list->moves[i];
    }
    qsort(out, n, sizeof(uint32_t), move_cmp);
    return n;
} /* collect_sorted */


// prints a sorted move array as space-separated "<from><to>" coordinates
static void print_move_array(uint32_t *moves, int n) {
    int i;

    for (i = 0; i < n; i++) {
        print_square(moves[i] & 0xFF);
        print_square((moves[i] >> 8) & 0xFF);
        printf(" ");
    }
    printf("\n");
} /* print_move_array */


// Compares the bitboard and mailbox generators at this node and, if they agree,
// recurses into every child to 'depth'. Returns 1 on the first divergence,
// printing the offending position's FEN and both move lists.
static int crosscheck_node(Game *game, int depth, uint32_t *path, int ply) {
    MoveList bb;                        // bitboard generator output
    MoveList mb;                        // mailbox oracle output
    uint32_t a[MOVELIST_CAPACITY];      // sorted bitboard moves
    uint32_t b[MOVELIST_CAPACITY];      // sorted mailbox moves
    int na, nb;                         // move counts
    int mismatch;                       // set when the two disagree
    int i;                              // loop index
    char fen[128];                      // FEN of a mismatching node

    generate_moves(game, &bb);
    generate_moves_mailbox(game, &mb);

    na = collect_sorted(&bb, a);
    nb = collect_sorted(&mb, b);

    // compare the two sorted move sets
    mismatch = (na != nb);
    for (i = 0; i < na && !mismatch; i++) {
        if (a[i] != b[i]) {
            mismatch = 1;
        }
    }

    if (mismatch) {
        printf("PATH: ");
        for (i = 0; i < ply; i++) {
            print_square(path[i] & 0xFF);
            print_square((path[i] >> 8) & 0xFF);
            printf(" ");
        }
        printf("\n");
        game_to_fen(game, fen);
        printf("MISMATCH: %s\n", fen);
        printf("  bitboard (%d): ", na);
        print_move_array(a, na);
        printf("  mailbox  (%d): ", nb);
        print_move_array(b, nb);
        return 1;
    }

    if (depth <= 1) {
        return 0;
    }

    // both agree here; descend using the (identical) bitboard move list
    for (i = 0; i < (int) bb.length; i++) {
        Game clone;
        memcpy(&clone, game, sizeof(Game));
        move_piece(&clone, bb.moves[i]);
        change_turn(&clone);
        path[ply] = bb.moves[i];
        if (crosscheck_node(&clone, depth - 1, path, ply + 1)) {
            return 1;
        }
    }
    return 0;
} /* crosscheck_node */


int run_crosscheck(Game *game, int depth) {
    uint32_t path[64];

    printf("Running crosscheck (bitboard vs mailbox) to depth %d:\n\n", depth);

    if (crosscheck_node(game, depth, path, 0)) {
        printf("\nCrosscheck FAILED: generators disagree (see mismatch above).\n");
        return 1;
    }

    printf("Crosscheck passed: generators agree at every node to depth %d.\n", depth);
    return 0;
} /* run_crosscheck */
