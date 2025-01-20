#include <stdio.h>
#include <stdint.h>
#include "chess_terminal.h"
#include "player.h"
#include "game.h"
#include "piece.h"
#include "io.h"
#include "ai.h"
#include "movelist.h"
#include "rules.h"
#include "move.h"
#include "utils.h"

void chess_terminal() {
    printf("\nInteractive Chess Terminal:\n");
    while (1) {
        print_options();
        
        printf("Enter option: ");
        uint32_t option = get_user_option();

        if (option == EXIT_INPUT) {
            return;
        }

        execute_option(option);
    }
}


void execute_option(uint32_t option) {
    switch(option) {
        case 1:	{           // user vs user
            Game *game = create_game();
            set_player_w(game, USER);
            set_player_b(game, USER);
            set_default_board(game);

            game_loop(game);
            
            delete_game(game);
            break;
        
        }
        case 2:	{           // user vs computer
            Game *game = create_game();
            set_player_w(game, USER);
            set_player_b(game, COMPUTER);
            set_default_board(game);

            game_loop(game);
            
            delete_game(game);
            break;
        
        }
        case 3:	{           // computer vs computer
            Game *game = create_game();
            set_player_w(game, COMPUTER);
            set_player_b(game, COMPUTER);
            set_default_board(game);

            game_loop(game);
            
            delete_game(game);
            break;
        }

        case 4:	{           // perft move generation
            break;
        
        }

        case 5:	{           // perft move generation (with FEN)
            Game *game = load_fen_game("./fen/move_generation_fen.txt");
            print_board(game);
            print_possible_moves(game);
            delete_game(game);
            break;

        }

        case 8:	{           // settings
            break;

        }

        // extra cases for testing things quickly
        case 101: {         // move test
            printf("Enter move: ");
            uint16_t move = get_user_move();
            printf("Entered move: %u\n", move);
            break;
        }
        
        case 102: {	        // movelist test
            MoveList *movelist = create_movelist();
            for (int i = 0; i < 10; i++) {
                add_move(movelist, i);
            }
            delete_movelist(movelist);
            break;
        }

        case 103: {         // piece values test
            Game *game = create_game();
            set_player_w(game, USER);
            set_player_b(game, USER);
            set_default_board(game);

            printf("Piece values\n");
            printf("    a1: %u\n", game->board[0]);
            printf("    a8: %u\n", game->board[7]);
            printf("    b2: %u\n", game->board[9]);
            printf("    b5: %u\n", game->board[12]);
            printf("    b7: %u\n", game->board[14]);

            delete_game(game);
            break;
        }
        
        default: {
            printf("Not a supported option!\n");
            break;
        }
    }
}


void print_options() {
    printf("\n");
    printf("Options:\n");
    printf("1 - User vs User\n");
    printf("2 - User vs Computer\n");
    printf("3 - Computer vs Computer\n");
    printf("4 - Perft move generation\n");
    printf("5 - Perft move generation (with FEN)\n");
    printf("8 - Settings\n");
    printf("9 - Exit\n");
    printf("\n");
}


void game_loop(Game *game) {
    while (1) {
        // game state codes:
        // 0 - game is normal, continue
        // 1 - game is done, exit loop
        uint8_t game_state = game_tick(game);

        if (game_state == 1) {
            break;
        }
    }
}


uint8_t game_tick(Game *game) {
    // get move
    uint32_t move = 0;
    if (!game->move) {
        print_board(game);
        if (game->player_w == USER) {
            printf("\n");
            move = get_valid_user_move(game);
        }
        else if (game->player_w == COMPUTER) {
            move = get_computer_move(game, PLAYERW);
        }
    }
    else  {
        print_board_reverse(game);
        if (game->player_b == USER) {
            printf("\n");
            move = get_valid_user_move(game);
        }
        else if (game->player_b == COMPUTER) {
            move = get_computer_move(game, PLAYERB);
        }
    }

    printf("\n");

    // make move
    move_piece(game, move);

    // set up for next iteration
    change_turn(game);
    
    uint8_t kings = 0;
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (is_king(game->board[i])) {
            kings++;
        }
    }

    if (kings != 2) {
        return 1;
    }

    // check game state (returns for loop exiting)
    return 0;
}

uint32_t get_valid_user_move(Game *game) {
    printf("Enter move: ");
    uint32_t move = (uint32_t) get_user_move();

    while (!is_legal_move(game, move)) {
        printf("Invalid move!\n");
        printf("Enter move: ");
        move = (uint32_t) get_user_move();
    }
    
    // post move flag adding
    if (!game->move) {
        move |= MOVE_WHITE_MASK;
    }
    else {
        move |= MOVE_BLACK_MASK;
    }

    // printf("Move: 0x%x\n", move);
    return move;
}


void dump_game_info(Game *game) {
    printf("GAME INFO\n");
    printf("    move: %d\n", game->move);
    printf("    castle_kingside_w:   %d\n", game->castle_kingside_w);
    printf("    castle_queenside_w:  %d\n", game->castle_queenside_w);
    printf("    castle_kingside_b:   %d\n", game->castle_kingside_b);
    printf("    castle_queenside_b:  %d\n", game->castle_queenside_b);
    printf("    w_en_passant_square: %d\n", game->w_en_passant_square);
    printf("    b_en_passant_square: %d\n", game->b_en_passant_square);
}
