#include <errno.h>
#include "xmalloc.h"
#include "selector.h"

#define NS_MAX_FD (1024)

struct fired_event {
	int mask;
	int fd;
};

struct event_handler {
	void (*handle)(int fd, void *data, int mask);
	int mask;
	void *data;
};

struct selector {
	struct event_handler eh[NS_MAX_FD];
	struct fired_event fe[NS_MAX_FD];
	int maxfd;
	int stop;
	void *state;
};

#include "ns_poll.c"

int sel_add_file(struct selector *sel,
                 int fd,
                 void (*handle)(int fd, void *data, int mask),
                 int mask, void *data)
{
	struct event_handler *eh;
	if (fd >= NS_MAX_FD)
		return -1;
	if (sel_add_event(sel, fd, mask) < 0)
		return -1;

	eh = &sel->eh[fd];
	eh->mask = mask;
	eh->handle = handle;
	eh->data = data;
	if (fd > sel->maxfd)
		sel->maxfd = fd;

	return 0;
}

void sel_del_file(struct selector *sel, int fd, int mask)
{
	struct event_handler *eh;

	if (fd >= NS_MAX_FD)
		return;
	eh = &sel->eh[fd];
	if (eh->mask == SE_NONE)
		return;
	eh->mask &= ~mask;
	sel_del_event(sel, fd, mask);
}

void sel_stop(struct selector *sel)
{
	sel->stop = 1;
}

static void sel_process(struct selector *sel)
{
	int numevents;
	int i;

	numevents = sel_poll(sel, SE_TIME_INF);
	for (i = 0; i < numevents; i++) {
		int fd = sel->fe[i].fd;
		int mask = sel->fe[i].mask;
		struct event_handler *eh = &sel->eh[fd];
		int cur_mask = eh->mask & mask;

		eh->handle(fd, eh->data, cur_mask);
	}
}

void sel_loop(struct selector *sel)
{
	while (!sel->stop)
		sel_process(sel);
}

struct selector *sel_new(void)
{
	struct selector *sel;

	sel = zmalloc(sizeof(*sel));
	sel_state_create(sel);
	sel->maxfd = -1;

	return sel;
}

void sel_release(struct selector *sel)
{
	sel_state_free(sel);
	xfree(sel);
}
