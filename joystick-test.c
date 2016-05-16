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

int player_started = 0;

int main()
{
	int count = 0, rc;

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
			
		if (ch == KEY_LEFT && player_started == 0) {
			ch = 'q';
		} else if (ch == KEY_LEFT && player_started == 1) {
			rc = system("~/src/ctv/dbuscontrol.sh stop");
			logi("dbus.stop. rc: %d\r", rc);
			player_started = 0;
		} else if (ch == KEY_RIGHT && player_started == 0) {
			rc = system("omxplayer --win 100,100,640,480 -s ~/b/ctvb/1.mp4 >/dev/null &");
			logi("start omxplayer. rc: %d\r", rc);
			player_started = 1;
		} else if (ch == KEY_RIGHT && player_started == 1) {
			rc = system("~/src/ctv/dbuscontrol.sh seek 60000000");
			logi("dbus.seek. rc: %d\r", rc);
		}
	}

	endwin();
}
