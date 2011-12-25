#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Set to 115200N81 */
static void serial_init(int fd)
{
	struct termios termctl;

	if (tcgetattr(fd, &termctl) < 0)
		return;

	cfmakeraw(&termctl);
	cfsetospeed(&termctl, B115200);
	cfsetispeed(&termctl, B115200);
	termctl.c_cflag &= ~(CSTOPB);
	termctl.c_cflag &= ~(CSIZE);
	termctl.c_cflag |= CS8;
	termctl.c_cflag &= ~(PARENB);
	termctl.c_cflag &= ~(CLOCAL);
	termctl.c_cflag &= ~(HUPCL);
	termctl.c_cflag |= CREAD;
	termctl.c_cflag &= ~(CRTSCTS);
	termctl.c_iflag &= ~(IXON | IXOFF | IXANY);
	termctl.c_iflag |= IGNBRK;
	termctl.c_cc[VMIN] = 1;
	termctl.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &termctl);
}

int serial_new(const char *path)
{
	int fd;
	int flags;

	fd = open(path, O_RDWR| O_NOCTTY |O_NDELAY);
	if (fd < 0)
		return -1;
	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags & (~O_NDELAY));

	serial_init(fd);

	return fd;
}

void serial_release(int fd)
{
	close(fd);
}
