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
	va_list args, cp;

	va_start(args, fmt);
	va_copy(cp, args);

	wattron(log_win, COLOR_PAIR(2));
	mvwaddstr(log_win, log_row, 2, "                                                                  ");
	wmove(log_win, log_row, 3);
	vwprintw(log_win, fmt, args);
	wattroff(log_win, COLOR_PAIR(2));
	wrefresh(log_win);

	va_end (args);

	va_start(args, fmt);
	va_copy(cp, args);

	char msg[1000];
	vsnprintf(msg, 999, fmt, args);
	logfatal("%s", msg);

	va_end (args);
}
