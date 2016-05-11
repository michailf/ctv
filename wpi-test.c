#include <stdio.h>
#include <err.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64
#define SIZE  4

int gpio_export(int gpio)
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

int gpio_set_edge(int gpio, char *edge)
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

int gpio_fd_open(int gpio)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);

	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		perror("gpio/fd_open");
	}
	return fd;
}

int gpio_set_active_low(int gpio, int alow_flag)
{
	int fd, len;
	char buf[MAX_BUF];

	len = snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/active_low", gpio);

	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		perror("gpio/active_low");
		return fd;
	}

	if (alow_flag)
		write(fd, "1", 1);
	else
		write(fd, "0", 1);

	close(fd);
	return 0;
}

static int count = 0;
int
wait_event(int *fds)
{
	struct pollfd fdset[5];
	int timeout_ms = 300000;
	int i;

	memset((void*)fdset, 0, sizeof(fdset));

	for (i = 0; i < SIZE; i++) {
		fdset[i].fd = fds[i];
		fdset[i].events = POLLPRI | POLLERR;
		fdset[i].revents = 0;
	}
	
	fdset[4].fd = 0;
	fdset[4].events = POLLIN;
	fdset[4].revents = 0;

	printf("%d. entering poll. fds[4] = %d\n", count++, fdset[4].fd);
	int rc = poll(fdset, 5, timeout_ms);

	printf("poll: rc: %d\n", rc);

	if (rc < 0) {
		err(1, "poll() failed: %d, %s", rc, strerror(rc));
		return -1;
	}

	for (i = 0; i < 5 && rc > 0; i++) {	
		if (fdset[i].revents == 0)
			continue;
		printf("%d. revents: 0x%02X\n", i, fdset[i].revents);
		printf("    POLLPRI: 0x%02X\n", fdset[i].revents & POLLPRI);
		printf("    POLLERR: 0x%02X\n", fdset[i].revents & POLLERR);
		printf("    POLLIN: 0x%02X\n", fdset[i].revents & POLLIN);
		char buf[64];
//		lseek(fds[i], 0, SEEK_SET);
		int len = read(fdset[i].fd, buf, 64);
		printf("    len: %d, %02X,%02X, errno: %d\n\n", len, buf[0], buf[1], errno);
		rc--;
	}

	return 0;
}

int main()
{
	int i, rc;
	int pins[SIZE] = { 17, 18, 27, 22  };
	int fds[SIZE] = { 0, 0, 0, 0 };
	
	printf("pins    ", pins[i]);
	for (i = 0; i < SIZE; i++) {
		printf(" %2d", pins[i]);
	}
	printf("\n");
	printf("export  ", pins[i]);

	for (i = 0; i < SIZE; i++) {
		rc = gpio_export(pins[i]);
		printf(" %2d", rc);
	}
	printf("\n");

	printf("edge    ");
	for (i = 0; i < SIZE; i++) {
		rc = gpio_set_edge(pins[i], "rising");
		printf(" %2d", rc);
	}
	printf("\n");
	
	printf("low     ");
	for (i = 0; i < SIZE; i++) {
		rc = gpio_set_active_low(pins[i], 0);
		printf(" %2d", rc);
	}
	printf("\n");

	printf("fds     ");
	for (i = 0; i < SIZE; i++) {
		fds[i] = gpio_fd_open(pins[i]);
		printf(" %2d", fds[i]);
	}
	printf("\n");

	printf("waiting 300 sec for event\n");
	
	while (1) {
		wait_event(fds);
		usleep(50000);
	}
}
