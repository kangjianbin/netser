#ifndef _X_MALLOC_H
#define _X_MALLOC_H

#include <malloc.h>
#include "clog.h"
static inline void *xmalloc(size_t sz)
{
	void *p = malloc(sz);
	if (p == NULL)
		die("Allocate %zu failed\n", sz);
	return p;
}

static inline void* zmalloc(size_t sz)
{
	void *p = calloc(1, sz);
	if (p == NULL)
		die("Allocate %zu failed\n", sz);
	return p;
}

static inline void xfree(void *p)
{
	free(p);
}

#endif
