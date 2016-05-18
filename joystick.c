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
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);

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
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/active_low", gpio);

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
	int fd;
	char buf[MAX_BUF];

	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);

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

#define MAX_PINS 5
#define FD_SIZE (MAX_PINS+1) // pins and stdin

static bool inited = false;
static int pins[MAX_PINS] = { 17, 18, 27, 22, 23  };    /* BCM pins */
static int gpio[MAX_PINS] = {  0,  1,  2,  3,  4  };    /* gpio numbers */
static int fds[FD_SIZE] =         { -1, -1, -1, -1, -1, 0 };  /* descriptors */
static int keys[MAX_PINS] = { KEY_DOWN, KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_HOME };
static uint64_t last_press;
static int last_pin = -1;

static struct pollfd fdset[FD_SIZE]; /* pool structs for 5 joystick buttons and stdin */

static uint64_t
get_ms()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static bool
init_pins()
{
	int i, rc;

	for (i = 0; i < MAX_PINS; i++) {
		rc = gpio_export(pins[i]);
		if (rc != 0) {
			logwarn("cannot export pin %d", pins[i]);
			return false;
		}

		rc = gpio_set_dir(pins[i], 0);
		if (rc != 0) {
			logwarn("cannot set dir to in for pin %d", pins[i]);
			return false;
		}

		rc = gpio_set_edge(pins[i], "falling");
		if (rc != 0) {
			logwarn("cannot set falling edge for pin %d", pins[i]);
			return false;
		}

		rc = gpio_set_active_low(pins[i], 0);
		if (rc != 0) {
			logwarn("cannot set active low for pin %d", pins[i]);
			return false;
		}

		char pullup_cmd[100];
		snprintf(pullup_cmd, 99, "gpio mode %d up", gpio[i]);
		rc = system(pullup_cmd);
		if (rc != 0) {
			logwarn("cannot exec %s. rc: %d", pullup_cmd, rc);
			return false;
		}

		fds[i] = gpio_fd_open(pins[i]);
		if (fds[i] == -1) {
			logwarn("cannot open pin %d", pins[i]);
			return false;
		}

		fdset[i].fd = fds[i];
		fdset[i].events = POLLPRI|POLLERR;

		usleep(100000);
	}

	return true;
}

void
joystick_init()
{
	int i;
	bool last_res = false;

	fdset[MAX_PINS].events = POLLIN|POLLERR;
	last_press = get_ms();

	if (!exists(SYSFS_GPIO_DIR)) {
		logwarn("%s doesnt exist. Skip joystick initialization.", SYSFS_GPIO_DIR);
		return;
	}

	for (i = 0; i < MAX_PINS*5; i++) {
		last_res = init_pins();
		if (last_res)
			break;
	}

	if (!last_res) {
		logfatal("cannot init pins after %d attempts", MAX_PINS);
	}

	inited = true;
}

static int
wait_event(int timeout, int *key)
{
	char buf[64];
	int i, rc, ch = -1, pin = -1;

	for (i = 0; i < FD_SIZE; i++) {
		fdset[i].revents = 0;
	}

	rc = poll(fdset, FD_SIZE, timeout);
	if (rc < 0)
		logfatal("poll() failed: %d, %s", rc, strerror(rc));

	for (i = 0; i < FD_SIZE && rc > 0; i++) {
		if (fdset[i].revents == 0)
			continue;

		lseek(fdset[i].fd, 0, SEEK_SET);
		ssize_t was_read = read(fdset[i].fd, buf, 64);
//		logi("i: %d, was_read: %d, b: %d,%d,%d\r", i, was_read, buf[0], buf[1], buf[1], buf[2]);
		rc--;
		ch = (i == MAX_PINS) ? buf[was_read-1] : keys[i];
		pin = i;
	}
	
	if (ch != -1) {
		*key = ch;
	}
	
	return pin;
}

int debug = 0;

int
joystick_getch()
{
	int pin1, key1 = -1;

	if (last_pin != MAX_PINS) {
		wait_event(10, &key1);
		uint64_t now = get_ms();
		if (debug)
			logi("now: %llu last: %llu\r", now, last_press);

		while (now - last_press < 300) {
			wait_event(100, &key1);
			now = get_ms();
			if (debug)
				logi("now: %llu last: %llu, diff: %llu\r", now, last_press, now - last_press);
		}
		wait_event(1, &key1);
		now = get_ms();
		if (debug)
			logi("now: %llu last: %llu\r", now, last_press);
	}

	pin1 = wait_event(60000, &key1);
	last_press = get_ms();
	last_pin = pin1;

	return key1;
}
