/** @file */

#ifndef SUDOKU_H
#define SUDOKU_H

#include "dlx.h"

#define NCOLS (81 * 4)
#define NROWS (81 * 9)
#define NTYPES 4

typedef enum {
    CELL_ID,
    ROW_ID,
    COL_ID,
    REGION_ID
} constraint_type;

/** @brief Data structures for dlx representation of a 9x9 sudoku */
typedef struct {
    hnode root;
    hnode headers[NCOLS];
    int   ids[NCOLS];
    node  nodes[NROWS][NTYPES];
} sudoku_dlx;

typedef struct {
    int    constraint_id;  /**< see sudoku.c */
    size_t solution_id;    /**< see sudoku.c */
    int    nchoices;       /**< number of other possible choices at the time */
} sudoku_hint;

int     sudoku_solve(const char *puzzle, char *buf);
size_t  sudoku_nsolve(const char *puzzle, char *buf, size_t n);
int     sudoku_solve_hints(const char *puzzle, sudoku_hint hints[]);
size_t  hint2cells(sudoku_hint *hint, int cell_ids[]);
void    hint2rcn(sudoku_hint *hint, int *r, int *c, int *n);

#endif
