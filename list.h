#ifndef _LIST_H
#define _LIST_H

#ifndef _MSC_VER
#include <stddef.h>
#endif
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type *)0)->member)
#endif

#ifndef container_of
#define container_of(ptr, type, member) \
	((type *)((char *)(ptr) - offsetof(type, member)))
#endif

struct list {
	struct list *next, *prev;
};

#define LIST_INIT(name)  { &(name), &(name) }
#define LIST(name) \
	struct list name = LIST_INIT(name)

#define list_entry(ptr, type, member)			\
	((type *)( (char *)(ptr) - offsetof(type, member)))

static inline void init_list(struct list *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(struct list *list,
                              struct list *prev,
                              struct list *next)
{
	next->prev = list;
	list->next = next;
	list->prev = prev;
	prev->next = list;
}

static inline void list_add(struct list *list,
                            struct list *head)
{
	__list_add(list, head, head->next);
}

static inline void list_add_tail(struct list *list,
                                 struct list *head)
{
	__list_add(list, head->prev, head);
}

static inline void __list_del(struct list *prev,
                              struct list *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list *list)
{
	__list_del(list->prev, list->next);
	list->prev = NULL;
	list->next = NULL;
}

static inline void list_del_init(struct list *entry)
{
	__list_del(entry->prev, entry->next);
	init_list(entry);
}

static inline void list_move(struct list *list, struct list *head)
{
	__list_del(list->prev, list->next);
	list_add(list, head);
}

static inline void list_move_tail(struct list *list, struct list *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}

static inline int list_empty(struct list *head)
{
	return head->next == head;
}

#define list_for_each(ptr, head) \
	for((ptr) = (head)->next; (ptr) != (head); (ptr) = (ptr)->next)

#define list_for_each_safe(pos, n, head)		      \
	for(pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)


/**
 * hlist from kernel
 */
struct hlist_node {
	struct hlist_node *next, **pprev;
};

struct hlist {
	struct hlist_node *first;
};

#define HLIST_INIT { .first = NULL }
#define HLIST(name) struct hlist name = HLIST_INIT
static inline void init_hlist(struct hlist *h)
{
	h->first = NULL;
}

static inline void init_hlist_node(struct hlist_node *n)
{
	n->next = NULL;
	n->pprev = NULL;
}

static inline int hlist_unhashed(const struct hlist_node *n)
{
	return !n->pprev;
}

static inline int hlist_empty(const struct hlist *h)
{
	return !h->first;
}

static inline void hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;

	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hlist_del_init(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		hlist_del(n);
		init_hlist_node(n);
	}
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist *h)
{
	struct hlist_node *first = h->first;

	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

#define hlist_entry(ptr, type, member) container_of(ptr, type, member)
#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos; pos = pos->next)
#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && (n = pos->next, 1); pos = n)

#endif
