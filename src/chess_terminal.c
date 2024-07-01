#include <stdio.h>
#include <stdint.h>
#include "chess_terminal.h"

void chess_terminal() {
	printf("\nInteractive Chess Terminal:\n");
	while (1) {
		print_options();
		
		printf("Enter option: ");
		uint32_t input = get_input();

		if (input == EXIT_INPUT) {
			return;
		}

		execute_input(input);
	}
}


uint32_t get_input() {
	uint32_t input;
	scanf("%u", &input);
	return input;
}


void execute_input(uint32_t input) {
	switch(input) {
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

		default:
			printf("Not a supported input!\n");
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
