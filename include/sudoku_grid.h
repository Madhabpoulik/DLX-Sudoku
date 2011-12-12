#ifndef SUDOKU_GRID_H
#define SUDOKU_GRID_H

typedef struct {
    unsigned char val;
    unsigned char flags;
} SudokuCell;

typedef struct SudokuGrid {
    int flags;
    int undo_list[81];
    int undo_pos;
    SudokuCell cells[81];
} SudokuGrid;

void init_board(SudokuGrid *board);

char *get_values(SudokuGrid *board, char values[]);
char *get_givens(SudokuGrid *board, char givens[]);
int  set_value(SudokuGrid *board, int r, int c, int val);
int  get_value(SudokuGrid *board, int r, int c);

void toggle_fix_mode(SudokuGrid *board);
int  is_fixed(SudokuGrid *board);
int  is_cell_fixed(SudokuGrid *board, int r, int c);

int undo_board(SudokuGrid *board);
void clear_board(SudokuGrid *board);

#endif
