#include <stdio.h>
#include <stdlib.h>
#include "sudoku.h"

int main(int argc, char *argv[])
{
    char puzzle[82];
    char solution[82];
    if (NULL != fgets(puzzle, sizeof(puzzle), stdin)) {
        if (sudoku_solve(puzzle, solution) != -1) {
            printf("%s\n", solution);
            exit(0);
        } else {
            fprintf(stderr, "No solution found.\n");
            exit(1);
        }
    }
    return 0;
}

