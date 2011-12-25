#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include "../selector.h"
#include "../serial.h"

#define SER "/dev/ttyUSB0"

struct ser_ctrl {
	int ser;
	sel_t sel;
};

static void on_serial(int fd, void *data, int mask)
{
	char buf[4096];
	int len;
	struct ser_ctrl *ctrl = data;

	len = read(fd, buf, sizeof(buf));
	if (len < 0) {
		fprintf(stderr, "Read from serial failed\n");
		sel_stop(ctrl->sel);
	} else {
		write(1, buf, len);
	}
}

static void on_read(int fd, void *data, int mask)
{
	char buf[4096];
	int len;
	struct ser_ctrl *ctrl = data;

	len = read(fd, buf, sizeof(buf));
	if (len < 0) {
		fprintf(stderr, "Read from stdin failed\n");
		sel_stop(ctrl->sel);
	} else {
		write(ctrl->ser, buf, len);
	}
}

static struct termios orig_term;
static int is_raw;
static void exit_raw(void)
{
	if (is_raw) {
		tcsetattr(0, TCSANOW, &orig_term);
		is_raw = 0;
	}
}

static void init_raw(void)
{
	struct termios termctl;

	if (tcgetattr(0, &termctl) < 0)
		return;
	is_raw = 1;
	orig_term = termctl;
	atexit(exit_raw);
	cfmakeraw(&termctl);
	termctl.c_cflag &= ~(CLOCAL);
	termctl.c_cflag &= ~(HUPCL);
	termctl.c_cflag |= CREAD;
	termctl.c_cflag &= ~(CRTSCTS);
	termctl.c_iflag &= ~(IXON | IXOFF | IXANY);
	termctl.c_iflag |= IGNBRK;
	tcsetattr(0, TCSAFLUSH, &termctl);
}


int main(int argc, char *argv[])
{
	sel_t sel = sel_new();
	int ser = serial_new(SER);
	struct ser_ctrl ctrl;

	if (ser < 0) {
		fprintf(stderr, "Open %s failed\n", SER);
		return -1;
	}
	init_raw();
	ctrl.sel = sel;
	ctrl.ser = ser;

	sel_add_file(sel, 0, on_read, SE_READABLE, &ctrl);
	sel_add_file(sel, ser, on_serial, SE_READABLE, &ctrl);

	sel_loop(sel);
	sel_release(sel);
	serial_release(ser);

	return 0;
}
