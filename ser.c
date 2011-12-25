#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"

#define MIN_PORT (2000)
static ser_t ser;
static void do_stop(int sig)
{
	ser_stop(ser);
}

static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s dev port [tel_mode]\n", name);
	fprintf(stderr, "    tcp_port should >= 2000\n");
}

static int get_int(char *s, int *v, int base)
{
	char *end_ptr;
	unsigned long long lv;

	lv = strtoll(s, &end_ptr, base);
	if (*end_ptr != '\0' || end_ptr == s)
		return -1;

	*v = (int)lv;

	return 0;
}

static int get_port(char *port)
{
	int p;

	if (get_int(port, &p, 0) < 0) {
		fprintf(stderr, "Invalidate port: %s\n", port);
		exit(1);
	}
	if (p < MIN_PORT || p > 65535) {
		fprintf(stderr, "Port number should between %d-65535\n",
		        MIN_PORT);
		exit(1);
	}

	return p;
}

static int get_mode(char *mode)
{
	int m;

	if (get_int(mode, &m, 0) < 0) {
		fprintf(stderr, "Invalidate mode: %s\n", mode);
		exit(1);
	}

	return m;
}

int main(int argc, char *argv[])
{
	const char *dev;
	int port;
	int mode;

	if (argc < 3) {
		usage(argv[0]);
		exit(1);
	}

	dev = argv[1];
	port = get_port(argv[2]);
	/* default to telnet mode */
	if (argc == 3)
		mode = 1;
	else
		mode = get_mode(argv[3]);

	signal(SIGPIPE, SIG_IGN);
	signal(SIGTERM, do_stop);
	ser = ser_new(dev, port, mode);
	if (ser == NULL)
		return -1;
	ser_start(ser);

	return 0;
}
