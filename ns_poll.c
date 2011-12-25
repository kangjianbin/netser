#include <sys/epoll.h>
#include <unistd.h>

struct sel_state {
	int ep_fd;
	struct epoll_event events[NS_MAX_FD];
};

static int sel_state_create(struct selector *sel)
{
	struct sel_state *state;

	state = xmalloc(sizeof(struct sel_state));
	state->ep_fd = epoll_create(NS_MAX_FD);
	if (state->ep_fd == -1)
		die("%s: create epoll failed\n", __func__);

	sel->state = state;

	return 0;
}

static void sel_state_free(struct selector *sel)
{
	struct sel_state *state = sel->state;

	close(state->ep_fd);
	xfree(sel->state);
}

static int sel_add_event(struct selector *sel, int fd, int mask)
{
	struct sel_state *state = sel->state;
	struct epoll_event ee;
	int old_mask = sel->eh[fd].mask;
	int op = old_mask == SE_NONE?
		EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events = 0;
	mask |= old_mask;
	if (mask & SE_READABLE)
		ee.events |= EPOLLIN;
	if (mask & SE_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.u64 = 0;
	ee.data.fd = fd;
	return epoll_ctl(state->ep_fd, op, fd, &ee);
}

static void sel_del_event(struct selector *sel, int fd, int delmask)
{
	struct sel_state *state = sel->state;
	struct epoll_event ee;
	int mask = sel->eh[fd].mask & (~delmask);

	ee.events = 0;
	if (mask & SE_READABLE)
		ee.events |= EPOLLIN;
	if (mask & SE_WRITABLE)
		ee.events |= EPOLLOUT;
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if (mask == SE_NONE)
		epoll_ctl(state->ep_fd, EPOLL_CTL_DEL, fd, &ee);
	else
		epoll_ctl(state->ep_fd, EPOLL_CTL_MOD, fd, &ee);
}

static int sel_poll(struct selector *sel, int timeout)
{
	struct sel_state *state = sel->state;
	int ret;
	int numevents = 0;

	ret = epoll_wait(state->ep_fd, state->events,
	                 NS_MAX_FD, timeout);
	if (ret > 0) {
		int i;
		numevents = ret;

		for (i = 0; i < numevents; i++) {
			int mask = 0;
			struct epoll_event *e = &state->events[i];

			if (e->events & EPOLLIN)
				mask |= SE_READABLE;
			if (e->events & EPOLLOUT)
				mask |= SE_WRITABLE;
			sel->fe[i].fd = e->data.fd;
			sel->fe[i].mask = mask;
		}
	}

	return numevents;
}



