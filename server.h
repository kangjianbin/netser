#ifndef _CTRL_H
#define _CTRL_H

struct ser_server;
typedef struct ser_server* ser_t;

ser_t ser_new(const char *path, int port, int is_telnet);
void ser_release(ser_t ser);
void ser_stop(ser_t ser);
void ser_start(ser_t ser);

#endif
