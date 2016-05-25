#include "util.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "common/log.h"
#include "common/regexp.h"

static WINDOW *log_win;
static int log_row;
static bool log_dumb = false;

void
status_init(WINDOW *w, int row, bool dumb_term)
{
	log_win = w;
	log_row = row;
	log_dumb = dumb_term;
}

void
statusf(const char *fmt, ...)
{
	va_list args, cp;

	va_start(args, fmt);
	va_copy(cp, args);

	if (log_dumb) {
		vprintf(fmt, args);
	} else {
		wattron(log_win, COLOR_PAIR(2));
		mvwaddstr(log_win, log_row, 2, "                                                                  ");
		wmove(log_win, log_row, 3);
		vwprintw(log_win, fmt, args);
		wattroff(log_win, COLOR_PAIR(2));
		wrefresh(log_win);
	}

	va_end (args);

	va_start(args, fmt);
	va_copy(cp, args);

	char msg[1000];
	vsnprintf(msg, 999, fmt, args);
	logfatal("%s", msg);

	va_end (args);
}

int
match_chunks(const char *text, int n, regmatch_t *m, const char *start, const char *end)
{
	regex_t rex_start;
	regex_t rex_end;
	regmatch_t m1, m2;
	int rc = 0, i;
	int len = strlen(text);

	regex_compile(&rex_start, start);
	regex_compile(&rex_end, end);

	m1.rm_so = 0;
	m1.rm_eo = len;

	for (i = 0; i < n; i++) {
		rc = regexec(&rex_start, text, 1, &m1, REG_STARTEND);

		if (rc == REG_NOMATCH) {
			rc = 0;
			break;
		}

		if (rc != 0)
			break;

		m2.rm_so = m1.rm_eo;
		m2.rm_eo = len;

		rc = regexec(&rex_end, text, 1, &m2, REG_STARTEND);

		if (rc == REG_NOMATCH) {
			rc = 0;
			break;
		}

		if (rc != 0)
			break;

		m[i].rm_so = m1.rm_eo;
		m[i].rm_eo = m2.rm_so;

		m1.rm_so = m2.rm_eo;
		m1.rm_eo = len;
	}

	for (; i < n; i++) {
		m[i].rm_so = -1;
		m[i].rm_eo = -1;
	}

	regfree(&rex_start);
	regfree(&rex_end);

	return rc;
}

void
split_chunks(char *text, int n, const regmatch_t *m, char *chunks[])
{
	int i;

	for (i = 0; i < 10; i++) {
		if (m[i].rm_so == -1)
			break;

		chunks[i] = &text[m[i].rm_so];
		char *end = &text[m[i].rm_eo];
		*end = 0;
	}
}
