/** @file */

#ifndef SUDOKU_H
#define SUDOKU_H

#define NCOLS (81 * 4)
#define NROWS (81 * 9)
#define NTYPES 4

typedef enum {
    CELL_ID,
    ROW_ID,
    COL_ID,
    REGION_ID
} id_t;

int sudoku_solve(const char *puzzle, char *buf);

#endif
