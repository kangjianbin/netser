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

#include "xmalloc.h"
#include "clog.h"
#include "list.h"
#include "selector.h"
#include "server.h"
#include "serial.h"
#include "tn.h"

struct ser_server {
	struct list client_list;
	sel_t sel;
	int tcp_fd;
	int ser_fd;
	int is_telnet;
};

struct ser_client {
	struct list list;
	struct tn_contex tn;
	int fd;
	struct ser_server *ser;
};

static void client_on_read(int fd, void *data, int mask);

static int set_non_block(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int set_socket(int fd)
{
	int optval = 1;
	socklen_t optlen = sizeof(optval);

	if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0)
		return -1;

	/* Set keep alive */
	optval = 3;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &optval, optlen) < 0)
		return -1;
	optval = 5;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &optval, optlen) < 0)
		return -1;
	optval = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &optval, optlen) < 0)
		return -1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, optlen) < 0)
		return -1;
	set_non_block(fd);

	return 0;
}

static void __ser_release_client(struct ser_server *server,
                                 struct ser_client *client)
{
	clog(CL_DEBUG, "Free client: %d\n", client->fd);
	sel_del_file(server->sel, client->fd, SE_ALL);
	list_del(&client->list);
	shutdown(client->fd, SHUT_RDWR);
	close(client->fd);
	xfree(client);
}

static void client_do_write(void *data, void *buf, int len)
{
	struct ser_client *client = data;

	write(client->fd, buf, len);
}

static void client_on_read(int fd, void *data, int mask)
{
	char buf[4096];
	int rdlen;
	struct ser_client *client = data;
	struct ser_server *ser = client->ser;

	rdlen = read(fd, buf, sizeof(buf));
	if (rdlen <= 0 &&
	    !(errno == EAGAIN || errno == EINTR)) {

		clog(CL_ERROR, "Read from %d failed\n", fd);
		__ser_release_client(ser, client);
		return;
	}
	if (rdlen > 0) {
		if (ser->is_telnet)
			rdlen = tn_parse(&client->tn, buf, rdlen);

		write(ser->ser_fd, buf, rdlen);
	}
}

static void ser_add_client(struct ser_server *server, int fd)
{
	int ret;
	struct ser_client *client;

	clog(CL_DEBUG, "Add client: %d\n", fd);
	if (set_socket(fd) < 0) {
		clog(CL_ERROR, "Set client socket failed\n");
		return;
	}
	client = xmalloc(sizeof(*client));
	client->fd = fd;
	client->ser = server;
	ret = sel_add_file(server->sel, fd,
	                   client_on_read, SE_READABLE, client);
	if (ret < 0) {
		clog(CL_ERROR, "Add client %d to poll failed\n", fd);
		xfree(client);
		return;
	}

	if (server->is_telnet)
		tn_init(&client->tn, client_do_write, client);

	list_add_tail(&client->list, &server->client_list);
}

static void tcp_on_connect(int fd, void *data, int mask)
{
	int client;
	struct sockaddr addr;
	socklen_t alen = sizeof(addr);
	struct ser_server *server = data;

	client = accept(fd, &addr, &alen);
	if (client == -1) {
		clog(CL_ERROR, "Server accpet failed\n");
		return;
	}
	ser_add_client(server, client);
}

static int ser_tcp_write_all(struct ser_server *ser, void *buf, int len)
{
	char obuf[2 * len];
	struct list *p;
	void *wbuf;
	int olen;

	if (ser->is_telnet) {
		olen = tn_prepare_send(buf, len, obuf, sizeof(obuf));
		if (olen < 0) {
			clog(CL_ERROR,
			     "Internal error: prepare telnet data failed\n");
			return -1;
		}
		wbuf = obuf;
	} else {
		wbuf = buf;
		olen = len;
	}

	list_for_each(p, &ser->client_list) {
		struct ser_client *client;

		client = list_entry(p, struct ser_client, list);
		write(client->fd, wbuf, olen);
	}

	return 0;
}

static void ser_failed_port(struct ser_server *ser, int fd)
{
	char *buf = "Can't read from serial port, exiting\n";
	int len = strlen(buf);

	ser_tcp_write_all(ser, buf, len);
	clog(CL_ERROR, buf);
	ser_stop(ser);
}

static void ser_on_read(int fd, void *data, int mask)
{
	char buf[4096];
	int rdlen;
	struct ser_server *ser = data;

	rdlen = read(fd, buf, sizeof(buf));
	if (rdlen <= 0 &&
	    !(errno == EAGAIN || errno == EINTR)) {

		clog(CL_ERROR, "Read from %d failed\n", fd);
		ser_failed_port(ser, fd);
		return;
	}

	if (rdlen > 0) {
		int ret;

		ret = ser_tcp_write_all(ser, buf, rdlen);
		if (ret < 0)
			ser_stop(ser);
	}
}

ser_t ser_new(const char *path, int port, int is_telnet)
{
	ser_t ser;
	int fd;
	int tcp_fd;
	sel_t sel;
	extern int tcp_start(int port);

	fd = serial_new(path);
	if (fd == -1) {
		clog(CL_ERROR, "Can't open %s\n", path);
		return NULL;
	}
	tcp_fd = tcp_start(port);
	if (tcp_fd < 0)
		goto out_serial;

	ser = xmalloc(sizeof(struct ser_server));
	sel = sel_new();
	if (sel_add_file(sel, fd, ser_on_read, SE_READABLE, ser) < 0)
		goto out_sel;
	if (sel_add_file(sel, tcp_fd, tcp_on_connect,
	                 SE_READABLE, ser) < 0)
		goto out_sel;
	init_list(&ser->client_list);
	ser->tcp_fd = tcp_fd;
	ser->ser_fd = fd;
	ser->sel = sel;
	ser->is_telnet = is_telnet;

	return ser;

out_sel:
	sel_release(sel);
	close(tcp_fd);
out_serial:
	close(fd);
	return NULL;
}

void ser_release(ser_t ser)
{
	struct list *p;

	sel_release(ser->sel);
	list_for_each(p, &ser->client_list) {
		struct ser_client *client;

		client = list_entry(p, struct ser_client, list);
		__ser_release_client(ser, client);
	}
	close(ser->tcp_fd);
	close(ser->ser_fd);
	xfree(ser);
}

void ser_stop(ser_t ser)
{
	sel_stop(ser->sel);
}

void ser_start(ser_t ser)
{
	sel_loop(ser->sel);
	ser_release(ser);
}
