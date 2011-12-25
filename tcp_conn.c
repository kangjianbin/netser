#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "clog.h"

static int set_non_block(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void set_server_socket(int fd)
{
	int used = 1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &used, sizeof(used));
	set_non_block(fd);
}

int tcp_start(int port)
{
	int ret;
	struct sockaddr_in saddr;
	int fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		clog(CL_ERROR, "Create socket failed\n");
		return -1;
	}

	set_server_socket(fd);
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(port);
	ret = bind(fd, (struct sockaddr *)&saddr, sizeof(saddr));

	if (ret < 0) {
		clog(CL_ERROR, "Bind to socket failed\n");
		goto bad_fd;
	}
	ret = listen(fd, 5);
	if (ret < 0) {
		clog(CL_ERROR, "Listen on socket failed\n");
		goto bad_fd;
	}

	return fd;

bad_fd:
	close(fd);

	return -1;
}

