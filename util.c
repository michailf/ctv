#include "util.h"
#include <stdarg.h>
#include <stdlib.h>
#include "common/log.h"

static WINDOW *log_win;
static int log_row;

void
status_init(WINDOW *w, int row)
{
	log_win = w;
	log_row = row;
}

void
statusf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	wattron(log_win, COLOR_PAIR(2));
	mvwaddstr(log_win, log_row, 2, "                                                                  ");
	wmove(log_win, log_row, 3);
	vwprintw(log_win, fmt, args);
	wattroff(log_win, COLOR_PAIR(2));
	wrefresh(log_win);

	va_end (args);
	exit(1);
}
