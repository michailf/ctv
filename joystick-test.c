#include <stdio.h>
#include "joystick.h"

int main()
{
	int count = 0;
	joystick_init();

	int ch = -1;
	while (ch != 'q') {
		ch = joystick_getch();
		printf("%d ch: %d\n", count++, ch);
	}
}
