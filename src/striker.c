#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chess_terminal.h"
#include "game.h"
#include "ai.h"

#define VERSION "0.0.1"

int main(int argc, char **argv) {
    printf("Striker Chess Engine\n");
    printf("Version: %s\n\n", VERSION);

    // command line modes:
    //   --validate            run move generation validation suite
    //   --perft <depth> [--fen <fen>]
    int perft_depth = -1;
    const char *fen = NULL;
    int validate = 0;
    int bench = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--validate") == 0) {
            validate = 1;
        }
        else if (strcmp(argv[i], "--bench") == 0) {
            bench = 1;
        }
        else if (strcmp(argv[i], "--perft") == 0 && i + 1 < argc) {
            perft_depth = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "--fen") == 0 && i + 1 < argc) {
            fen = argv[++i];
        }
    }

    if (validate) {
        int failures = run_validation();
        return failures == 0 ? 0 : 1;
    }

    if (bench) {
        run_benchmark();
        return 0;
    }

    if (perft_depth >= 0) {
        Game *game;
        if (fen != NULL) {
            game = parse_fen(fen);
        }
        else {
            game = create_game();
            set_default_board(game);
        }

        print_board(game);
        printf("\n");

        run_perft_test(game, (uint32_t) perft_depth);

        delete_game(game);
        return 0;
    }

    chess_terminal();

    return 0;
}
