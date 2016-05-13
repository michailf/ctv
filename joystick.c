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
static int keys[4] = { KEY_DOWN, KEY_LEFT, KEY_UP, KEY_RIGHT };
static struct pollfd fdset[5]; /* pool structs for 4 joystick buttons and stdin */

static int64_t last_press = 0;
static int last_index = -1;

static int64_t
get_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void
joystick_init()
{
	int i, rc;

	last_press = get_ms();
	fdset[4].events = POLLIN|POLLERR;

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

		rc = gpio_set_edge(pins[i], "falling");
		if (rc != 0)
			logfatal("cannot set falling edge for pin %d", pins[i]);

		rc = gpio_set_active_low(pins[i], 1);
		if (rc != 0)
			logfatal("cannot set active low for pin %d", pins[i]);
		
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
	}

	inited = true;
}

static int
wait_event(int timeout, int *key)
{
	char buf[64];
	int i, rc;

	for (i = 0; i < 5; i++) {
		fdset[i].revents = 0;
	}

	rc = poll(fdset, 5, timeout);
	if (rc < 0)
		logfatal("poll() failed: %d, %s", rc, strerror(rc));

	for (i = 0; i < 5 && rc > 0; i++) {
		if (fdset[i].revents == 0)
			continue;

		lseek(fdset[i].fd, 0, SEEK_SET);
		ssize_t was_read = read(fdset[i].fd, buf, 64);
//		logi("i: %d, was_read: %d, buf[0]: %d\r", i, was_read, buf[was_read-1]);
		rc--;
		*key = (i == 4) ? buf[was_read-1] : keys[i];
		
		return i;
	}
	
	return -1;
}

static int count = 0;

int
joystick_getch()
{
	if (count++ > 1000)
		return 'q';

	int pin1, pin2, key1 = -1, key2 = -1;
	
	pin1 = wait_event(60000, &key1);
//	logi("poll1: %d\r", pin1);
	if (pin1 == -1)
		return -1;
	if (pin1 == 4)
		return key1;

	usleep(100000);
	int v = -1;
	gpio_get_value(pins[pin1], &v);
//	logi("v: %d\r", v);
	if (v == 0) {
		pin2 = pin1;
		key2 = keys[pin2];
	}

	wait_event(5, &key2);

//	logi("poll2: %d\r", pin2);
	if (pin1 == pin2) {
		switch (key2) {
			case KEY_UP:
				logi("UP\r");
				break;
			case KEY_DOWN:
				logi("DOWN\r");
				break;
			case KEY_LEFT:
				logi("LEFT\r");
				break;
			case KEY_RIGHT:
				logi("RIGHT\r");
				break;
		}
		return key2;
	}

	return -1;
}
