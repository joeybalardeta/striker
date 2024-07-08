#include <stdio.h>
#include <stdint.h>
#include "chess_terminal.h"
#include "player.h"
#include "game.h"
#include "io.h"
#include "ai.h"
#include "movelist.h"

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
            break;
        
        }
        case 3:	{           // computer vs computer
            break;
        }

        case 4:	{           // perft move generation
            break;
        
        }

        case 5:	{           // perft move generation (with FEN)
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
    printf("\n");
    // get move
    uint16_t move = 0;
    if (!game->move) {
        print_board(game);
        if (game->player_w == USER) {
            printf("\n");
            printf("Enter move: ");
            move = get_user_move();
        }
        else if (game->player_w == COMPUTER) {
            move = get_computer_move(game);
        }
    }
    else  {
        print_board_reverse(game);
        if (game->player_b == USER) {
            printf("\n");
            printf("Enter move: ");
            move = get_user_move();
        }
        else if (game->player_b == COMPUTER) {
            move = get_computer_move(game);
        }
    }

    printf("\n");
    
    // make move
    move_piece(game, move);

    // set up for next iteration
    change_turn(game);
    
    // check game state (returns for loop exiting)
    return 0;
}
