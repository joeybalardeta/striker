#include <stdio.h>
#include "chess_terminal.h"

#define VERSION "0.0.1"

int main(int argc, char **argv) {
    printf("Striker Chess Engine\n");
    printf("Version: %s\n\n", VERSION);

    chess_terminal();

    return 0;
}
