CROSS_COMPILE =
CFLAGS = -Wall -O2 -g 
LDFLAGS += 

ifeq ($(T),debug)
CFLAGS += -Dclog_level=CL_MAX
else
CFLAGS += -Dclog_level=CL_INFO
endif

sources = ../selector.c ../serial.c ../server.c ../tcp_conn.c ../tn.c
progs = test_sel tser

all: $(progs)

test_sel: $(sources) test_sel.c
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "  LD     $@"

tser: $(sources) tser.c
	@$(CC) $(CFLAGS) $^ -o $@
	@echo "  LD     $@"

clean:
	@for i in $(progs); do \
	$(RM) $$i; echo "  CLEAN  $$i"; done
