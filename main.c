#include <json-c/json.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncursesw/ncurses.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <locale.h>
#include "common/struct.h"
#include "version.h"
#include "api.h"
#include "util.h"

static void
synopsis()
{
	printf("usage: ctv [-hdptvc] [name]\n");
}

static void
usage()
{
	printf("ctv. Console video player for etvnet.com.\n");
	printf("Serge Voilokov, 2016.\n");
	synopsis();
	printf("options:\n"
	       "  -a    activate tv box on etvnet.com\n"
	       "  -d    dump debug output to stderr\n"
	       "  -v    print version\n"
	       );
}

static void
version()
{
	printf("ctv. Console video player for etvnet.com.\n");
	printf("Serge Voilokov, 2016.\n");
	printf("version %s\n", app_version);
	printf("date %s\n", app_date);
}

static bool activate_box = false;

static void
init(int argc, char **argv)
{
	int ch;
	char cache_dir[500];

	while (optind < argc) {

		ch = getopt(argc, argv, "ahv");
		if (ch == -1)
			continue;
		switch (ch) {
			case 'a':
				activate_box = true;
				break;
			case 'h':
				usage();
				exit(1);
			case 'v':
				version();
				exit(1);
		}
	}

	snprintf(cache_dir, 500, "%s/.cache/", getenv("HOME"));
	mkdir(cache_dir, 0700);
	strcat(cache_dir, "etvcc/");
	mkdir(cache_dir, 0700);
}

struct movie_list *list; /* read from my favorites */

enum scroll_mode {
	eNames,
	eNumbers
};

struct ui
{
	int height;
	int width;
	WINDOW *win;
	WINDOW *win_desc;
	enum scroll_mode scroll;
};

static void
init_ui(struct ui *ui)
{
	setlocale(LC_ALL, "");
	initscr();
	start_color();
	getmaxyx(stdscr, ui->height, ui->width);
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_WHITE, COLOR_RED);
	init_pair(3, COLOR_YELLOW, COLOR_BLUE);
	refresh();
	noecho();
	cbreak();
	ui->win = newwin(ui->height - 20, 70, 2, 1);
	ui->win_desc = newwin(10, 70, 60, 2);
	box(ui->win, 0, 0);
	box(ui->win_desc, 0, 0);
	curs_set(0);
}

struct ui ui;

static void
draw_list()
{
	int i;
	for (i = 0; i < list->count; i++) {

		if (i == list->sel && ui.scroll == eNames) {
			wattron(ui.win, COLOR_PAIR(1));
			mvwaddstr(ui.win, i+2, 2, "[");
		} else {
			mvwaddstr(ui.win, i+2, 2, " ");
		}

		struct movie_entry *e = list->items[i];
		mvwaddstr(ui.win, i+2, 4, e->name);

		if (i == list->sel && ui.scroll == eNames) {
			mvwaddstr(ui.win, i+2, 37, "]");
			wattroff(ui.win, COLOR_PAIR(1));
		} else {
			mvwaddstr(ui.win, i+2, 37, " ");
		}

		mvwaddstr(ui.win, i+2, 39, "      ");

		if (i == list->sel && ui.scroll == eNumbers) {
			wattron(ui.win, COLOR_PAIR(1));
			mvwprintw(ui.win, i+2, 39, "%d/%d     ", e->sel + 1, e->children_count);
			mvwaddstr(ui.win, i+2, 37, "[");
			mvwaddstr(ui.win, i+2, 45, "]");
			wattroff(ui.win, COLOR_PAIR(1));
		} else {
			mvwprintw(ui.win, i+2, 39, "%d/%d     ", e->sel + 1, e->children_count);
		}

		mvwaddstr(ui.win, i+2, 47, e->on_air);
	}
}

static void
next_movie()
{
	struct movie_entry *e = list->items[list->sel];
	if (e->children_count > 0) {
		if (++e->sel >= e->children_count)
			e->sel = 0;
	}
}

static void
prev_movie()
{
	struct movie_entry *e = list->items[list->sel];
	if (e->children_count > 0) {
		if (--e->sel < 0)
			e->sel = e->children_count - 1;
	}
}

static void
next_number()
{
	if (++list->sel >= list->count)
		list->sel = 0;
}

static void
prev_number()
{
	if (--list->sel < 0)
		list->sel = list->count - 1;
}

static void
exit_handler()
{
	timeout(-1);
//	attron(COLOR_PAIR(2));
//	mvaddstr(10, 10, "CTV exited. Press any key.");
	getch();
	flushinp();
	endwin();
}

static void
print_status(const char *msg)
{
	wattron(ui.win, COLOR_PAIR(3));
	mvwaddstr(ui.win, ui.height - 22, 2, "                                                                  ");
	mvwaddstr(ui.win, ui.height - 22, 3, msg);
	wattroff(ui.win, COLOR_PAIR(3));
	wrefresh(ui.win);
}

static void
run_player(const char *url)
{
	char cmd[2000];
	char msg[100];

	if (strcmp("/home/pi", getenv("HOME")) == 0)
		snprintf(cmd, 1999, "omxplayer --live --blank --key-config /home/pi/bin/omxp_keys.txt '%s'", url);
	else
		snprintf(cmd, 1999, "mplayer -msglevel all=0 -cache-min 64 '%s' 2>/dev/null 1>&2", url);

	int rc = system(cmd);
	snprintf(msg, 99, "video stopped: %d", rc);
	print_status(msg);

}

static void
play_movie()
{
	print_status("Loading movie...");

	struct movie_entry *e = list->items[list->sel];
	if (e->children_count == 0) {
		char *url = etvnet_get_stream_url(e->id, 400);
		if (etvnet_errno != 0)
			statusf("play_movie: %s", etvnet_error());
		run_player(url);
		free(url);
		return;
	}

	struct movie_entry *child = etvnet_get_movie(e->id, e->sel);
	if (etvnet_errno != 0)
		statusf("part %d: %s", e->sel, etvnet_error());

	char *url = etvnet_get_stream_url(child->id, 400);
	if (etvnet_errno != 0)
		statusf("play_movie[%d]: %s", e->sel, etvnet_error());
	print_status("Playing movie...");
	run_player(url);
	free(child);
	free(url);
}

static int
wait_event()
{
	struct timeval tv = { 100L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv);
}

static int
read_joystick()
{
	return -1;
}

static void
activate_tv_box()
{
	char *code = etvnet_get_activation_code();
	if (code == NULL)
		err(1, "cannot get activation code: %s", etvnet_error());

	printf("Enter activation code on etvnet.com/Активация STB:\n\n    %s\n", code);
	printf("After entering the code hit ENTER on this box.\n");
	getch();

	etvnet_authorize(code);
	if (etvnet_errno != 0)
		err(1, "cannot active box: %s", etvnet_error());

	printf("Activated successfully.\n");
	exit(0);
}

int
main(int argc, char **argv)
{
	int quit = 0;
	init(argc, argv);

	if (activate_box) {
		activate_tv_box();
		return 0;
	}

	init_ui(&ui);
	status_init(ui.win, ui.height - 22);
	atexit(exit_handler);
	timeout(0);

	print_status("Connecting to etvnet.com");
	etvnet_init();
	if (etvnet_errno != 0)
		statusf("%s", etvnet_error());

	print_status("Loading favorites");
	list = etvnet_load_favorites();
	if (etvnet_errno != 0)
		statusf("%s", etvnet_error());

	print_status("UP,DOWN:move RIGHT:select LEFT:exit");

	while (!quit) {
		draw_list();
		wrefresh(ui.win);

		int has_events = wait_event();
		if (has_events == 0)
			continue;

		int ch = -1;

		if (has_events == 1)
			ch = getch();
		else if (has_events == 2)
			ch = read_joystick();

		switch (ch) {
			case 'q': case 'Q':
				quit = 1;
				break;
			case 'A':
			case KEY_UP:
				if (ui.scroll == eNumbers)
					prev_movie();
				else
					prev_number();
				break;
			case 'B':
			case KEY_DOWN:
				if (ui.scroll == eNumbers)
					next_movie();
				else
					next_number();
				break;
			case 'D':
			case KEY_LEFT:
				if (ui.scroll == eNames)
					quit = 1;
				else if (ui.scroll == eNumbers) {
					ui.scroll = eNames;
					print_status("UP,DOWN:move RIGHT:select LEFT:exit");
				}
				break;
			case 'C':
			case KEY_RIGHT:
				if (ui.scroll == eNumbers)
					play_movie();
				else if (ui.scroll == eNames) {
					ui.scroll = eNumbers;
					print_status("UP,DOWN:move RIGHT:play LEFT:back");
				}
				break;
		}
	}
}



































