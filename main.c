#include <json-c/json.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ncurses.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <locale.h>
#include "common/struct.h"
#include "version.h"
#include "api.h"

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
	       "  -d,--debug           dump debug output to stderr\n"
	       "  -v,--version         print version\n"
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

static void
init(int argc, char **argv)
{
	int ch;
	char cache_dir[500];

	while (optind < argc) {

		ch = getopt(argc, argv, "hv");
		if (ch != -1) {
			switch (ch) {
				case 'h':
					usage();
					exit(1);
				case 'v':
					version();
					exit(1);
			}
		}
	}

	snprintf(cache_dir, 500, "%s/.cache/", getenv("HOME"));
	mkdir(cache_dir, 0700);
	strcat(cache_dir, "etvcc/");
	mkdir(cache_dir, 0700);
	api_init(cache_dir);
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
	for (int i = 0; i < list->count; i++) {

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
on_exit()
{
	printw("\nPress ENTER\n");
	getch();
	endwin();
}

int
main(int argc, char **argv)
{
	int quit = 0;
	init(argc, argv);
	init_ui(&ui);
	atexit(on_exit);

	list = load_favorites();

	while (!quit) {
		draw_list();
		wrefresh(ui.win);
		int ch = getch();

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
				if (ui.scroll == eNumbers)
					ui.scroll = eNames;
				break;
			case 'C':
			case KEY_RIGHT:
				if (ui.scroll == eNames)
					ui.scroll = eNumbers;
				break;
		}
	}

//	usage();
//	synopsis();
}



































