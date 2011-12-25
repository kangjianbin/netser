CROSS_COMPILE =
CFLAGS = -Wall -O2 -g 
LDFLAGS += 
progs = ser

ifeq ($(T),debug)
CFLAGS += -Dclog_level=CL_MAX
else
CFLAGS += -Dclog_level=CL_INFO
endif

ser-objs = selector.o serial.o server.o tcp_conn.o ser.o tn.o

all:
clean::
	@$(MAKE) -C test clean


-include config.mk
