#include <stdio.h>
#include <stdint.h>
#include "chess_terminal.h"
#include "io.h"

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
        case 1:				// user vs user
            break;
        
        case 2:				// user vs computer
            break;
        
        case 3:				// computer vs computer
            break;
        
        case 4:				// perft move generation
            break;
        
        case 5:				// perft move generation (with FEN)
            break;

        case 8:				// settings
            break;

        // extra cases for testing things quickly
        case 101:			// move test
            printf("Enter move: ");
            uint16_t move = get_user_move();
            printf("Entered move: %u\n", move);
            break;

        default:
            printf("Not a supported option!\n");
            break;
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
