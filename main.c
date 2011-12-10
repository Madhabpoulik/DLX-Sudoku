#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "sudoku.h"

static const char *optstring = "vc:";

static int      g_verbose_flag = 0;
static size_t   g_count        = 0;

static void usage(int argc, char *argv[])
{
    fprintf(stdout,

"USAGE: %s [-n count] < {puzzle} \n\n"

"OPTIONS\n"
"  -c count\tcheck for up to c solutions before returning one\n"
"\t\tReturns 2 if more than one solution found.\n"
"\t\tWith -v, print number of solutions found (up to c) to stderr\n"
"  -v\t\tSubject to change in the future; for now,\n"
"\t\tonly affects output when combined with -c\n"

"\nStandard Input\n"
"\t\tA single sudoku puzzle in the format of an 81 character string\n"
"\t\tis read from standard input.\n"

            , argv[0]);
}

int main(int argc, char *argv[])
{
    int     c;
    size_t  n;
    char    puzzle[82];
    char    solution[82];

    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            case 'c':
                g_count = atoi(optarg);
                break;
            case 'v':
                g_verbose_flag = 1;
                break;
            case '?':
                usage(argc, argv);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (NULL != fgets(puzzle, sizeof(puzzle), stdin)) {
        if (g_count > 0) {
            n = sudoku_nsolve(puzzle, solution, g_count);
            if (g_verbose_flag)
                fprintf(stderr, "%lu\n", (unsigned long) n);
            if (n > 0)
                printf("%s\n", solution);
            exit(2);
        } else {
            if (sudoku_solve(puzzle, solution)) {
                printf("%s\n", solution);
                exit(EXIT_SUCCESS);
            } else {
                if (g_verbose_flag)
                    fprintf(stderr, "No solution found.\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

