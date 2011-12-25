#include <signal.h>
#include <stdio.h>
#include "../server.h"

static ser_t ser;
static void do_stop(int sig)
{
	ser_stop(ser);
}

int main(int argc, char *argv[])
{

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, do_stop);
	ser = ser_new("/dev/ttyUSB0", 4307, 1);
	if (ser == NULL)
		return -1;

	ser_start(ser);

	return 0;
}
