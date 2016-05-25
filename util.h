#include <ncursesw/ncurses.h>
#include <regex.h>

void status_init(WINDOW *w, int row, bool dumb_term);
void statusf(const char *fmt, ...);

/* regexec for a text chunks from start to end */
int match_chunks(const char *text, int n, regmatch_t *m, const char *start, const char *end);

/* set null terminator to the matched ends. Fill chunks[] with start pointers */
void split_chunks(char *text, int n, const regmatch_t *m, char *chunks[]);
