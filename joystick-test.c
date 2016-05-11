#include <stdio.h>
#include <ncursesw/ncurses.h>
#include "joystick.h"
#include "common/log.h"

int main()
{
	int count = 0;

	log_set(stderr);
	joystick_init();

	initscr();
	noecho();
	cbreak();

	int ch = -1;
	while (ch != 'q') {
		ch = joystick_getch();
		printf("%d %d(%c)\r\n", count++, ch, ch);
	}

	endwin();
}
