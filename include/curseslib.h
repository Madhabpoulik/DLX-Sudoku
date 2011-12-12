#ifndef CURSESLIB_H
#define CURSESLIB_H
#include <curses.h>

void wgrid(WINDOW *w, int y, int y_interval, int y_repeat,
                      int x, int x_interval, int x_repeat, bool draw_border);
#endif
