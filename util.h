#include <ncursesw/ncurses.h>

void status_init(WINDOW *w, int row, bool dumb_term);
void statusf(const char *fmt, ...);
