#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "ai.h"
#include "game.h"
#include "player.h"
#include "piece.h"
#include "rules.h"
#include "movelist.h"
#include "utils.h"

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

    uint32_t move = get_movelistentry(possible_moves, rand() % possible_moves->length)->move;

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
}


uint32_t perft(Game *game, int8_t depth) {
    static uint8_t max_depth = 0;

    if (depth > max_depth) {
        max_depth = depth;
    }

    MoveList *possible_moves = get_possible_moves(game);

    if (depth == 1) {
        return possible_moves->length;
    }

    uint32_t total_moves = 0;

    MoveListEntry* mle = possible_moves->first;
    for (int i = 0; i < possible_moves->length; i++) {
        Game *clone = clone_game(game);
        move_piece(clone, mle->move);
        change_turn(clone);
        uint32_t moves = perft(clone, depth - 1);
        delete_game(clone);

        #ifdef DEBUG_PERFT
        if (depth == max_depth) {
            print_move(mle->move);
            printf(": %d\n", moves);
        }
        #endif

        mle = mle->next;

        total_moves += moves;
    }

    return total_moves;
}
