#include <stdio.h>
#include <ncursesw/ncurses.h>
#include "joystick.h"
#include "common/log.h"

static char *keystr[1024] = {
	[13] = "CR",
	['A'] = "A",
	['B'] = "B",
	['C'] = "C",
	['D'] = "D",
	['q'] = "q",
	[KEY_DOWN] = "DOWN",
	[KEY_LEFT] = "LEFT",
	[KEY_UP] = "UP",
	[KEY_RIGHT] = "RIGHT",
	[KEY_HOME] = "HOME"
};

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
		if (ch > 0)
			logi("%d ch: %3d %s\r", count++, ch, keystr[ch]);
		if (ch == 13)
			logi("=====================================\r");
	}

	endwin();
}
