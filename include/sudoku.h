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
} id_t;

/** @brief Data structures for dlx representation of a 9x9 sudoku */
typedef struct {
    hnode root;
    hnode headers[NCOLS];
    int   ids[NCOLS];
    node  nodes[NROWS][NTYPES];
} sudoku_dlx;

int     sudoku_solve(const char *puzzle, char *buf);
size_t  sudoku_nsolve(const char *puzzle, char *buf, size_t n);

#endif
