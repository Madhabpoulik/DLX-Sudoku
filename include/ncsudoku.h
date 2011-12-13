#ifndef NCSUDOKU_H
#define NCSUDOKU_H

#include <curses.h>
#include "sudoku_grid.h"

typedef struct {
    int y;
    int x;
    int flags;
} NcSudokuCell;

typedef struct NcSudokuGrid {
    WINDOW *win;
    int y;              /**< top left corner y position */
    int x;              /**< top left corner x position */
    int cell_height;
    int cell_width;
    NcSudokuCell cells[81];
    SudokuGrid *board;
} NcSudokuGrid;

void nc_init_board(NcSudokuGrid *ncboard, WINDOW *win, SudokuGrid *board,
                   int y, int x, int cell_height, int cell_width);

void draw_board(NcSudokuGrid *ncboard);
void draw_cell(NcSudokuGrid *ncboard, int r, int c);

void highlight_cell(NcSudokuGrid *ncboard, int r, int c);
void unhighlight_cell(NcSudokuGrid *ncboard, int r, int c);
void unhighlight_all(NcSudokuGrid *ncboard);

void move_cursor(NcSudokuGrid *ncboard, int r, int c);

void move_cursor_left(NcSudokuGrid *ncboard, int *r, int *c);
void move_cursor_down(NcSudokuGrid *ncboard, int *r, int *c);
void move_cursor_up(NcSudokuGrid *ncboard, int *r, int *c);
void move_cursor_right(NcSudokuGrid *ncboard, int *r, int *c);

#endif
