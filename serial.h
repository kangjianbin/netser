#ifndef _SERIAL_H
#define _SERIAL_H


extern int serial_new(const char *path);
extern void serial_release(int fd);

#endif
