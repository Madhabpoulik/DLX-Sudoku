#include <stdarg.h>
#include <panel.h>
#include "ncsudoku.h"
#include "sudoku.h"

#define MSG_AREA_MAXY 10
#define MSG_AREA_MINX 48

static const char str_entry_mode[] = "Puzzle Entry mode";
static const char str_solve_mode[] = "Solver mode";
static const char str_invalid_puzzle[] =
"The entered puzzle cannot be accepted because it does not have a valid solution.";
static const char str_not_fixed[] = "Puzzle not yet fixed";
static const char str_help[] = "Keys\n"
"move: hjkl; numbers: 1-9; erase: 0,<space>; " "clear: c; undo: u;\n"
"fix givens: f; solve: s;\n"
"quit: q";
static const char str_not_unique[] = "Warning: the current puzzle has multiple solutions.\n"
"Hints will be disabled.";

/* global window variables */
static SudokuGrid   board;
static NcSudokuGrid ncboard;
static int boardy, boardx;
static int cellh, cellw;
static int winh, winw;
static int titley = 0;
static int titlex = 1;
static WINDOW *msg_area_border;
static WINDOW *msg_area;
static PANEL *panels[] = {NULL, NULL, NULL};

static void init_msg_area(void)
{
    int h, w;
    int y = 9 * cellh + 10 + boardy;

    getmaxyx(stdscr, h, w);
    h -= y;
    if (h > MSG_AREA_MAXY)
        h = MSG_AREA_MAXY;
    w = 9 * cellw + 10;
    if (w < MSG_AREA_MINX)
        w = MSG_AREA_MINX;

    msg_area_border = newwin(h, w, y, boardx);
    msg_area = newwin(h - 2, w - 2, y + 1, boardx + 1);
    box(msg_area_border, 0, 0);
}

static void init_panels(void) {
    panels[0] = new_panel(stdscr);
    panels[1] = new_panel(msg_area_border);
    panels[2] = new_panel(msg_area);
}

static void print_title_area(const char *fmt, ...)
{
    int savey, savex;
    va_list ap;
    va_start(ap, fmt);

    getyx(stdscr, savey, savex);
    move(titley, titlex);
    clrtoeol();
    vwprintw(stdscr, fmt, ap);
    va_end(ap);
    move(savey, savex);
}

static void print_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    werase(msg_area);
    wmove(msg_area, 0, 0);
    vwprintw(msg_area, fmt, ap);

    va_end(ap);
}

static void clear_msg(void)
{
    werase(msg_area);
}

int main(int argc, char *argv[])
{
    char         puzzle[82];
    sudoku_hint  hints[81];
    int ch;     /* getch */
    int i, t;   /* temp */
    int cr = 1; /* cursor position */
    int cc = 1; /* cursor position */
    int error_state = 0;

    boardy = 1;
    boardx = 1;
    cellh = 3;
    cellw = 7;

    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);

    getmaxyx(stdscr, winh, winw);
    init_msg_area();
    init_panels();

    /* set up and draw board */
    init_board(&board);
    nc_init_board(&ncboard, stdscr, &board, boardy, boardx, cellh, cellw);
    draw_board(&ncboard);
    print_title_area("%s", str_entry_mode);
    update_panels();
    doupdate();
    move_cursor(&ncboard, cr, cc);

    while ((ch = getch()) != 'q') {
        if (error_state) {
            clear_msg();
            error_state = 0;
        }
        switch (ch) {
            case '?':   /* show help */
                print_msg("%s", str_help);
                break;
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
            case 'f': toggle_fix_mode(&board);
                /* if entering fixed mode, validate and solve puzzle */
                if (get_givens(&board, puzzle) != NULL) {
                    /* if puzzle invalid */
                    if (!sudoku_solve_hints(puzzle, hints)) {
                        toggle_fix_mode(&board);
                        print_msg("Error: %s", str_invalid_puzzle);
                        error_state = 1;
                    } else { /* puzzle valid, but check uniqueness */
                        print_title_area("%s", str_solve_mode);
                        if (sudoku_nsolve(puzzle, NULL, 2) > 1) {
                            print_msg("%s", str_not_unique);
                            /* disable hints */
                        }
                    }
                } else {
                    print_title_area("%s", str_entry_mode);
                }
                /* toggle_fix_mode (un)bolds every char so refresh needed */
                draw_board(&ncboard);
                break;
            case 'u': t = undo_board(&board);   /* only works in fixed mode */
                if (t >= 0) {
                    cr = t / 9 + 1;
                    cc = t % 9 + 1;
                    draw_cell(&ncboard, cr, cc);
                }
                break;
            case 's':   /* solve puzzle if in fixed mode */
                if (!is_fixed(&board)) {
                    print_msg("%s: %s", str_not_fixed,
                            "press 'f' to fix the givens first.");
                    error_state = 1;
                    break;
                } /* else */
                for (i = 0; i < 81; i++) {
                    hint2rcn(hints + i, &cr, &cc, &t);
                    set_value(&board, cr, cc, t % 10 + '0');
                }
                draw_board(&ncboard);
        }
        update_panels();
        doupdate();
        move_cursor(&ncboard, cr, cc);
    }

    endwin();

    return 0;
}
