/**
 * @file
 * SudokuGrid is an interactive sudoku grid with two modes of operation.
 * In free input mode, you can set or clear any cell at will.  Once givens
 * are fixed, undo history recording begins, and the only allowed operation is
 * to fill in a blank cell or undo.  The board can be unfixed at any time, but
 * doing so wipes the given list immediately.  The values are stored as
 * characters, so 1 is represented by ASCII '1', not the value 1.
 */

#include <stddef.h>
#include "sudoku_grid.h"

/** grid and cell flags */
#define SC_FIXED     0x01

#define SG_FIXED     0x01

#define EMPTY_CELL_VAL ' '

/* hopefully the compiler will be smart enough to inline this by itself */
/** @brief convert row, column coordinate to an index from 0 to 80 */
static int rc2index(int r, int c)
{
    return 9 * (r - 1) + c - 1;
}

/** @brief initialize grid values */
void init_board(SudokuGrid *board)
{
    int i;
    SudokuCell *cell = board->cells;
    board->flags = 0;
    board->undo_pos = 0;
    for (i = 0; i < 81; i++, cell++) {
        cell->val = EMPTY_CELL_VAL;
        cell->flags = 0;
    }
}

/**
 * @brief fill values with board values.
 *
 * @param values    must have room for 82 char: 81 + null terminator
 * @return values
 * */
char *get_values(SudokuGrid *board, char values[])
{
    int i;
    char *c = values;
    SudokuCell *cell = board->cells;
    for (i = 0; i < 81; i++)
        *c++ = cell++->val;
    *c = '\0';
    return values;
}

/**
 * @brief fill givens with givens; operation is only valid if board is fixed
 *
 * @param givens    must have room for 82 char: 81 + null terminator; positions
 *                  corresponding to givens are filled, and other positions are
 *                  filled with a character not '1' - '9'
 * @return NULL if board givens not fixed, givens otherwise
 */
char *get_givens(SudokuGrid *board, char givens[])
{
    int i;
    char *c = givens;
    SudokuCell *cell = board->cells;
    if (!is_fixed(board)) 
        return NULL;
    for (i = 0; i < 81; i++, c++, cell++) {
        if (cell->flags & SC_FIXED)
            *c = cell->val;
        else
            *c = EMPTY_CELL_VAL;
    }
    *c = '\0';
    return givens;
}

/** 
 * @brief try to set cell value.  Operation will fail if attempting to change
 * or clear an existing cell value in fixed mode.
 *
 * @param val   must be between '1' to '9'.
 * @return -1 for illegal operation, 0 on success
 */
int set_value(SudokuGrid *board, int r, int c, int val) {
    int i = rc2index(r, c);
    SudokuCell *cell = board->cells + i;
    int v = cell->val;

    if (is_fixed(board)) {
        if (v >= '1' && v <= '9')   /* already filled */
            return -1;
        else if (val >= '1' && val <= '9') {
            board->undo_list[board->undo_pos++] = i;
            cell->val = val;
        }
    } else
        cell->val = val;
    return 0;
}

/** @return value at row r, column c */
int get_value(SudokuGrid *board, int r, int c)
{
    return board->cells[rc2index(r, c)].val;
}

/**
 * @brief toggle fixed mode.
 *
 * When fixed mode is set, every value in the board is marked as fixed (i.e.
 * treated as a given) and changes to those squares are forbidden.  Further, in
 * fixed mode undo history is kept, and no filled in values can be changed or
 * deleted.
 */
void toggle_fix_mode(SudokuGrid *board)
{
    int i, v;
    SudokuCell *cell = board->cells;
    if (is_fixed(board)) /* unfix all cells */
        for (i = 0; i < 81; i++, cell++)
            cell->flags &= ~ SC_FIXED;
    else {
        /* fix all cells that are filled in */
        for (i = 0; i < 81; i++, cell++) {
            v = cell->val;
            if (v >= '1' && v <= '9')
                cell->flags |= SC_FIXED;
        }
        /* clear undo history */
        board->undo_pos = 0;
    }
    board->flags ^= SG_FIXED;
}

/** @return nonzero if board is in fixed mode, 0 otherwise */
int is_fixed(SudokuGrid *board)
{
    return board->flags & SG_FIXED;
}

/** @return nonzero if board is in fixed mode and cell is fixed, 0 otherwise */
int is_cell_fixed(SudokuGrid *board, int r, int c)
{
    return is_fixed(board) && board->cells[rc2index(r, c)].flags & SC_FIXED;
}

/**
 * @brief Fixed mode only.  Undo last fill-in position.
 * @return 1D position of square undone, or -1 if nothing to undo or board is
 * not in fixed mode
 */
int undo_board(SudokuGrid *board)
{
    int i = 0;
    if (!is_fixed(board))
        return -1;
    if (board->undo_pos > 0) {
        (board->undo_pos)--;
        i = board->undo_list[board->undo_pos];
        board->cells[i].val = EMPTY_CELL_VAL;
    }
    return i;
}

/**
 * @brief in unfixed mode, clear all values in board; in fixed mode, undo all
 * entries.
 */
void clear_board(SudokuGrid *board)
{
    int i;
    SudokuCell *cell = board->cells;
    if (is_fixed(board)) {
        for (i = board->undo_pos; i > 0; i--) 
            undo_board(board);
    } else {
        for (i = 0; i < 81; i++, cell++)
            cell->val = EMPTY_CELL_VAL;
    }
}

