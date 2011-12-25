/**
 * A simple logger
 *
 * User should define clog_level macro or variable first.
 * Example:
 * int clog_level = CL_MAX; log all message
 * #define clog_level CL_NONE; to be quiet
 */
#ifndef CLOG_H
#define CLOG_H

#include <stdlib.h>

#ifndef ARCH_CLOG

#define die(fmt, ...) do {			\
	fprintf(stderr, "%s-%s: " fmt,		\
		__func__, __FILE__,		\
	        ##__VA_ARGS__);			\
	exit(1);				\
	} while (0)

#define clog(level, fmt, ...) do {			\
	if (level <= clog_level)			\
		fprintf(stderr, fmt,			\
		        ##__VA_ARGS__);			\
	} while (0)

#else

extern void die(const char *fmt, ...);
extern void clog(int level, const char *fmt, ...);

#endif /* !def ARCH_LOG */

enum {
	CL_ERROR	= 0,
	CL_WARN,
	CL_INFO,
	CL_DEBUG,
	CL_DEBUG_V,
	CL_MAX		= CL_DEBUG_V,
	CL_NONE		= -1,
};

#endif
