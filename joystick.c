#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <err.h>
#include <sys/time.h>
//#include <sys/ioctl.h>
#include <ncursesw/ncurses.h>
#include "common/fs.h"
#include "common/log.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64

static int
gpio_export(int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if (fd < 0) {
		perror("gpio/export");
		return fd;
	}

	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);

	return 0;
}

static int
gpio_set_dir(int gpio, int out_flag)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/direction");
		return fd;
	}

	if (out_flag) {
		write(fd, "out", 4);
	} else {
		write(fd, "in", 3);
	}

	close(fd);
	return 0;
}

static int
gpio_set_active_low(int gpio, int alow_flag)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/active_low", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/active_low");
		return fd;
	}

	if (alow_flag) {
		write(fd, "1", 1);
	} else {
		write(fd, "0", 1);
	}

	close(fd);
	return 0;
}

static int
gpio_set_edge(int gpio, char *edge)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/set-edge");
		return fd;
	}

	write(fd, edge, strlen(edge) + 1);
	close(fd);
	return 0;
}

static int
gpio_fd_open(int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

static int
gpio_get_value(int gpio, int *value)
{
	int fd, len;
	char buf[MAX_BUF];
	char ch;

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY);
	if (fd < 0) {
		perror("gpio/get-value");
		return fd;
	}

	read(fd, &ch, 1);

	if (ch != '0') {
		*value = 1;
	} else {
		*value = 0;
	}

	close(fd);
	return 0;
}

#define MAX_PINS 4

static bool inited = false;
static int pins[MAX_PINS] = { 17, 18, 27, 22  };    /* BCM pins */
static int gpio[MAX_PINS] = {  0,  1,  2,  3  };    /* gpio numbers */
static int fds[5] =         { -1, -1, -1, -1, 0 };  /* descriptors */
static struct pollfd fdset[5]; /* pool structs for 4 joystick buttons and stdin */

void
joystick_init()
{
	int i, rc;

	fdset[4].events = POLLIN|POLLERR;
	int flags = fcntl(fdset[5].fd, F_GETFL, 0);
	rc = fcntl(fdset[5].fd, F_SETFL, flags | O_NONBLOCK);
	if (rc == -1)
		logfatal("fcntl error: %d",errno);

	if (!exists(SYSFS_GPIO_DIR)) {
		logwarn("%s doesnt exist. Skip joystick initialization.", SYSFS_GPIO_DIR);
		return;
	}

	for (i = 0; i < MAX_PINS; i++) {
		rc = gpio_export(pins[i]);
		if (rc != 0)
			logfatal("cannot export pin %d", pins[i]);

		rc = gpio_set_dir(pins[i], 0);
		if (rc != 0)
			logfatal("cannot set dir to in for pin %d", pins[i]);

		rc = gpio_set_edge(pins[i], "rising");
		if (rc != 0)
			logfatal("cannot set rising edge for pin %d", pins[i]);

		rc = gpio_set_active_low(pins[i], 0);
		if (rc != 0)
			logfatal("cannot unset active low for pin %d", pins[i]);
		
		char pullup_cmd[100];
		snprintf(pullup_cmd, 99, "gpio mode %d up", gpio[i]);
		rc = system(pullup_cmd);
		if (rc != 0)
			logfatal("cannot exec %s. rc: %d", pullup_cmd, rc);

		fds[i] = gpio_fd_open(pins[i]);
		if (fds[i] == -1)
			err(1, "cannot open pin %d", pins[i]);

		fdset[i].fd = fds[i];
		fdset[i].events = POLLPRI|POLLERR;
		int flags = fcntl(fdset[i].fd, F_GETFL, 0);
		rc = fcntl(fdset[i].fd, F_SETFL, flags | O_NONBLOCK);
		if (rc == -1)
			logfatal("cannot exec %s. error: %d", pullup_cmd, errno);
	}

	inited = true;
}

static int64_t
get_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static int64_t last_press = 0;

int
joystick_getch()
{
	const int fdn = 5;
	int timeout_ms = 60000;
	int debounce_ms = 20;
	int i;

	int64_t elapsed_ms = get_ms() - last_press;
	if (elapsed_ms < debounce_ms) {
		usleep(debounce_ms * 1000 - elapsed_ms * 1000);
		char buf[64];
		for (i = 0; i < 5; i++) {
			lseek(fdset[i].fd, 0, SEEK_SET);
			ssize_t was_read = read(fdset[i].fd, buf, 64);
		}
	}

	for (i = 0; i < 5; i++) {
		fdset[i].revents = 0;
	}

	int rc = poll(fdset, fdn, timeout_ms);

	if (rc < 0) {
		logfatal("poll() failed: %d, %s", rc, strerror(rc));
		return -1;
	}

	int ch = -1;

	for (i = 0; i < fdn && rc > 0; i++) {
		if (fdset[i].revents == 0)
			continue;

		char buf[64];
		lseek(fdset[i].fd, 0, SEEK_SET);
		ssize_t was_read = read(fdset[i].fd, buf, 64);
		rc--;

		switch (i) {
			case 0:
				ch = KEY_UP;
				break;
			case 1:
				ch = KEY_DOWN;
				break;
			case 2:
				ch = KEY_LEFT;
				break;
			case 3:
				ch = KEY_RIGHT;
				break;
			case 4:
				{
				if (was_read == 3 && buf[0] == 27 && buf[1] == '[')
					ch = buf[2];
				else if (was_read == 1)
					ch = buf[0];
				else
					printf("was_read: %ld: 0x%02X,0x%02X,0x%02X\r\n",
				       		was_read, (uint8_t)buf[0], (uint8_t)buf[1], (uint8_t)buf[2]);
				}
				break;
		}

		last_press = get_ms();
	}

	return ch;
}
