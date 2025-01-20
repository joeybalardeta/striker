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

// debug define macros (enables compilation of printf statements)
// #define DEBUG_AI

uint32_t get_computer_move(Game *game, uint8_t player) {
    #ifdef DEBUG_AI
    printf("Calculating move for ");
    printf(player == PLAYERW ? "White" : "Black");
    printf("...\n");
    #endif

    MoveList *possible_moves = get_possible_moves(game, player);

    srand(time(NULL)); 

    uint32_t move = get_movelistentry(possible_moves, rand() % possible_moves->length)->move;

    return move;
}


void print_possible_moves(Game *game) {
    uint8_t player = !game->move ? PLAYERW : PLAYERB;
    printf("Getting possible moves for ");
    printf(player == PLAYERW ? "White" : "Black");
    printf("...\n");
    
    MoveList *possible_moves = get_possible_moves(game, player);

    print_movelist(possible_moves);
}
