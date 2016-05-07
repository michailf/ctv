#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

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

static int pins[MAX_PINS] = { 17, 18, 27, 22  };
static int fds[MAX_PINS] = { -1, -1, -1, -1 };

void joistick_init()
{
	int i, rc;

	for (i = 0; i < MAX_PINS; i++) {
		rc = gpio_export(pins[i]);
		if (rc != 0)
			err(1, "cannot export pin %d", pins[i]);

		rc = gpio_set_edge(pins[i], "rising");
		if (rc != 0)
			err(1, "cannot set rising edge for pin %d", pins[i]);

		rc = gpio_set_active_low(pins[i], 0);
		if (rc != 0)
			err(1, "cannot unset active low for pin %d", pins[i]);

		fds[i] = gpio_fd_open(pins[i]);
		if (fds[i] == -1)
			err(1, "cannot open pin %d", pins[i]);
	}
	
	while (1) {
		wait_event(fds);
		usleep(50000);
	}
}

int
wait_event(int bits)
{
	struct pollfd fdset[4];
	int timeout_ms = 300000;
	int i;

	memset((void*)fdset, 0, sizeof(fdset));

	for (i = 0; i < SIZE; i++) {
		fdset[i].fd = fds[i];
		fdset[i].events = POLLPRI | POLLERR;
	}

	int rc = poll(fdset, SIZE, timeout_ms);

	printf("poll: rc: %d\n", rc);

	if (rc < 0) {
		err(1, "poll() failed: %d, %s", rc, strerror(rc));
		return -1;
	}

	for (i = 0; i < SIZE && rc > 0; i++) {	
		if (fdset[i].revents == 0)
			continue;
		printf("%d. revents: 0x%02X\n", i, fdset[i].revents);
		printf("    POLLPRI: 0x%02X\n", fdset[i].revents & POLLPRI);
		printf("    POLLERR: 0x%02X\n", fdset[i].revents & POLLERR);
		char buf[64];
		lseek(fds[i], 0, SEEK_SET);
		int len = read(fds[i], buf, 64);
		printf("    len: %d, %02X,%02X\n\n", len, buf[0], buf[1]);
		rc--;
	}

	return 0;
}
