#ifndef SELECTOR_H
#define SELECTOR_H

#define SE_TIME_INF	(-1)

enum {
	SE_NONE,
	SE_READABLE	= 1,
	SE_WRITABLE	= 2,
	SE_ALL		= (SE_READABLE | SE_WRITABLE),
};

struct selector;
typedef struct selector* sel_t;
extern struct selector *sel_new(void);
extern void sel_release(struct selector *sel);
extern void sel_loop(struct selector *sel);
extern int sel_add_file(struct selector *sel,
                        int fd,
                        void (*handle)(int fd, void *data, int mask),
                        int mask, void *data);
extern void sel_del_file(struct selector *sel, int fd, int mask);
extern void sel_stop(struct selector *sel);

#endif
