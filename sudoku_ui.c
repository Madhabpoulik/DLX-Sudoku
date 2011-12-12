#include "ncsudoku.h"

int main(int argc, char *argv[])
{
    SudokuGrid board;
    NcSudokuGrid ncboard;
    int cr = 1; /* cursor position */
    int cc = 1; /* cursor position */
    int c, t;

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    /* set up and draw board */
    init_board(&board);
    nc_init_board(&ncboard, stdscr, &board, 1, 2, 3, 7);
    draw_board(&ncboard);
    move_cursor(&ncboard, cr, cc);
    touchwin(stdscr);
    refresh();

    while ((c = getch()) != 'q') {
        switch (c) {
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
                set_value(&board, cr, cc, c);
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
                /* redraw entire board and restore cursor position */
                draw_board(&ncboard);
                break;
            case 'u':
                t = undo_board(&board);
                if (t > 0) {
                    cr = t / 9 + 1;
                    cc = t % 9 + 1;
                    draw_cell(&ncboard, cr, cc);
                    move_cursor(&ncboard, cr, cc);
                }
                break;
        }
        refresh();
    }

    endwin();

    return 0;
}
