#include "ncsudoku.h"
#include "curseslib.h"

/** grid and cell flags */
#define NCSC_HIGHLIGHT  0x01
#define NCSC_FIXED      0x02;

#include "ncsudoku.h"

/** @brief convert row, column coordinate to an index from 0 to 80 */
static int rc2index(int r, int c)
{
    return 9 * (r - 1) + c - 1;
}

/**
 * @brief compute y x offsets of center of cell relative to top left corner,
 * and add them to the arguments
 */
static void cell_center_yx(NcSudokuGrid *ncboard, int *y, int *x)
{
    *y += ncboard->cell_height / 2;
    *x += ncboard->cell_width  / 2;
}

/** @brief Make 81 cells in the proper locations.  */
static void init_cells(NcSudokuGrid *ncboard)
{
    int i;
    int y = ncboard->y;
    int x = ncboard->x;
    int h = ncboard->cell_height;
    int w = ncboard->cell_width;
    NcSudokuCell *cell = ncboard->cells;

    /* calculate positions for all 81 cells */
    for (i = 0; i < 81; i++, cell++) {
        cell->y = y + 1 + (i / 9) * (h + 1);
        cell->x = x + 1 + (i % 9) * (w + 1);
        cell->flags = 0;
    }
}

/** @brief draw cell */
void draw_cell(NcSudokuGrid *ncboard, int r, int c)
{
    int i;
    NcSudokuCell *cell = ncboard->cells + rc2index(r, c);
    int v = get_value(ncboard->board, r, c);
    int y = cell->y;
    int x = cell->x;
    int h = ncboard->cell_height;
    int w = ncboard->cell_width;
    WINDOW *win = ncboard->win;

    /* display highlight cells in reverse video */
    if (cell->flags & NCSC_HIGHLIGHT)
        wattron(win, A_STANDOUT);
    else
        wattroff(win, A_STANDOUT);

    /* display fixed cells (givens) in bold */
    if (is_cell_fixed(ncboard->board, r, c))
        wattron(win, A_BOLD);
    else
        wattroff(win, A_BOLD);

    /* erase previous contents */
    for (i = 0; i < h; i++)
        mvwhline(win, y + i, x, ' ', w);
    /* write character */
    cell_center_yx(ncboard, &y, &x);
    mvwaddch(win, y, x, v);
    wmove(win, y, x);       /* put cursor back */
    wattrset(win, A_NORMAL);
}

/** @brief initialize ncboard members */
void
nc_init_board(NcSudokuGrid *ncboard, WINDOW *win, SudokuGrid *board,
              int y, int x, int cell_height, int cell_width)
{
    ncboard->win = win;
    ncboard->y = y;
    ncboard->x = x;
    ncboard->cell_height = cell_height;
    ncboard->cell_width = cell_width;
    ncboard->board = board;
    init_cells(ncboard);
}

/**
 * @brief redraw the board.  
 *
 * This should only need to be called at the beginning of the program to draw
 * the board for the first time as nothing in the external interface will ever
 * overwrite the board.
 */
void draw_board(NcSudokuGrid *ncboard)
{
    int i, j, y, x;
    int save_cursor_y, save_cursor_x;
    WINDOW *win = ncboard->win; 
    int h = ncboard->cell_height;
    int w = ncboard->cell_width;
    int by = ncboard->y;
    int bx = ncboard->x;
    getyx(win, save_cursor_y, save_cursor_x);
    /* outermost grid */
    wattron(win, A_BOLD);
    wgrid(win, by, 3 * (h + 1), 3, bx, 3 * (w + 1), 3, 1);
    wattroff(win, A_BOLD);
    /* small grids in each region */
    for (i = 0; i < 3; i++) {
        y = by + 1 + i * 3 * (h + 1);
        for (j = 0; j < 3; j++) {
            x = bx + 1 + j * 3 *(w + 1);
            wgrid(win, y, h + 1, 3, x, w + 1, 3, 0);

            /* erase the edges of the inner grid lines to make them easier
             * to distinguish */
            mvwaddch(win, y,             x + w,         ' ');   /* top, left */
            mvwaddch(win, y,             x + 2 * w + 1, ' ');   /* top, right */
            mvwaddch(win, y + 3 * h + 1, x + w,         ' ');   /* bottom, left */
            mvwaddch(win, y + 3 * h + 1, x + 2 * w + 1, ' ');   /* bottom, right */
            mvwaddch(win, y + h,         x,             ' ');   /* left, top */
            mvwaddch(win, y + 2 * h + 1, x,             ' ');   /* left, bottom */
            mvwaddch(win, y + h,         x + 3 * w + 1, ' ');   /* right, top */
            mvwaddch(win, y + 2 * h + 1, x + 3 * w + 1, ' ');   /* right, bottom */
        }
    }
    /* draw all cells */
    for (i = 1; i < 9 + 1; i++)
        for (j = 1; j < 9 + 1; j++)
            draw_cell(ncboard, i, j);
    wmove(win, save_cursor_y, save_cursor_x);
}

/** */
void highlight_cell(NcSudokuGrid *ncboard, int r, int c)
{
    ncboard->cells[rc2index(r, c)].flags |= NCSC_HIGHLIGHT;
    draw_cell(ncboard, r, c);
}

/** */
void unhighlight_cell(NcSudokuGrid *ncboard, int r, int c)
{
    ncboard->cells[rc2index(r, c)].flags &= ~ NCSC_HIGHLIGHT;
    draw_cell(ncboard, r, c);
}

/** @brief move cursor to cell */
void move_cursor(NcSudokuGrid *ncboard, int r, int c)
{
    NcSudokuCell *cell = ncboard->cells + rc2index(r, c);
    int y = cell->y;
    int x = cell->x;
    cell_center_yx(ncboard, &y, &x);
    wmove(ncboard->win, y, x);
}

void move_cursor_left(NcSudokuGrid *ncboard, int *r, int *c)
{
    if (*c == 1)
        *c = 9;
    else --*c;
    move_cursor(ncboard, *r, *c);
}

void move_cursor_down(NcSudokuGrid *ncboard, int *r, int *c)
{
    if (*r == 9)
        *r = 1;
    else ++*r;
    move_cursor(ncboard, *r, *c);
}

void move_cursor_up(NcSudokuGrid *ncboard, int *r, int *c)
{
    if (*r == 1)
        *r = 9;
    else --*r;
    move_cursor(ncboard, *r, *c);
}

void move_cursor_right(NcSudokuGrid *ncboard, int *r, int *c)
{
    if (*c == 9)
        *c = 1;
    else ++*c;
    move_cursor(ncboard, *r, *c);
}

