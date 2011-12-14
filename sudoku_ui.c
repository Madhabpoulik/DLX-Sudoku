#include "ncsudoku.h"
#include "sudoku.h"

int main(int argc, char *argv[])
{
    SudokuGrid   board;
    NcSudokuGrid ncboard;
    char         puzzle[82];
    sudoku_hint  hints[81];
    int cr = 1; /* cursor position */
    int cc = 1; /* cursor position */
    int ch;     /* getch */
    int i, t;   /* temp */
    /* int r, c, n;     * temp for row, col, val */

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    /* set up and draw board */
    init_board(&board);
    nc_init_board(&ncboard, stdscr, &board, 1, 1, 3, 7);
    draw_board(&ncboard);
    move_cursor(&ncboard, cr, cc);
    touchwin(stdscr);
    refresh();

    while ((ch = getch()) != 'q') {
        switch (ch) {
            case 'h':
                move_cursor_left(&ncboard, &cr, &cc);
                break;
            case 'j':
                move_cursor_down(&ncboard, &cr, &cc);
                break;
            case 'k':
                move_cursor_up(&ncboard, &cr, &cc);
                break;
            case 'l':
                move_cursor_right(&ncboard, &cr, &cc);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                set_value(&board, cr, cc, ch);
                draw_cell(&ncboard, cr, cc);
                break;
            case ' ':
            case 'd':
            case 0x08: /* ^H */
            case KEY_BACKSPACE:
                set_value(&board, cr, cc, ' ');     /* erase */
                draw_cell(&ncboard, cr, cc);
                break;
            case 'c':
                clear_board(&board);
                draw_board(&ncboard);
                break;
            case 'f':
                toggle_fix_mode(&board);
                if (get_givens(&board, puzzle) != NULL) {
                    if (!sudoku_solve_hints(puzzle, hints)) {
                        toggle_fix_mode(&board);
                        /* dialog invalid puzzle */
                    }
                }
                /* toggle_fix_mode (un)bolds every char so refresh needed */
                draw_board(&ncboard);
                break;
            case 'u':
                t = undo_board(&board);
                if (t >= 0) {
                    cr = t / 9 + 1;
                    cc = t % 9 + 1;
                    draw_cell(&ncboard, cr, cc);
                }
                break;
            case 's':
                if (!is_fixed(&board)) {
                    /* dialog */
                    break;
                } /* else */
                for (i = 0; i < 81; i++) {
                    hint2rcn(hints + i, &cr, &cc, &t);
                    set_value(&board, cr, cc, t % 10 + '0');
                }
                move_cursor(&ncboard, cr, cc);
                draw_board(&ncboard);
        }
        refresh();
    }

    endwin();

    return 0;
}
