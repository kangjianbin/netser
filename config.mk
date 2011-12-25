all:

CC := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip
AR ?= ar
HOSTCC ?= gcc
HOSTCXX ?= g++
HOSTSTRIP ?= strip

RM ?= rm -f
RMDIR ?= rmdir
T ?= release

ARFLAGS = rcs

ifeq ($(T), debug)
CFLAGS += -Wall -g -O2 -DDEBUG
CXXFLAGS += -Wall -g -O2 -DDEBUG
HOSTCFLAGS += -Wall -g -O2
HOSTCXXFLAGS += -Wall -g -O2
else
CFLAGS += -Wall -Wstrict-prototypes -fomit-frame-pointer -O2 \
		-fno-common 
CXXFLAGS += -Wall -fomit-frame-pointer -O2 -fno-common
LDFLAGS += -s
HOSTCFLAGS += -Wall -Wstrict-prototypes -fomit-frame-pointer -O2 \
		-fno-common
HOSTCXXFLAGS += -Wall -fomit-frame-pointer -O2 -fno-common
HOSTLDFLAGS += -s

endif
CFLAGS += -pipe
HOSTCFLAGS += -pipe

ifeq ($(Q),)
quiet := quiet_
else
quiet := 
endif

test.x86-objs = test.o
test3.x86-objs = test3.o

dot-target = $(dir $@).$(notdir $@)
depfile = $(dot-target).d
getm-objs = $(if $($(1)-objs),$($(1)-objs),$(1).o)
echo-cmd = @echo '  $($(quiet)cmd_$(1))';
cmd = @$(echo-cmd) $(cmd_$(1))

ifeq ($(findstring Win,$(OS)),Win)
gen-target = $(if 1,$(1).exe)
gen-targets = $(foreach m,$(1),$(addsuffix .exe,$(m)))
rm-suffix = $(patsubst %.exe,%,$(1))
else
gen-target = $(1)
gen-targets = $(1)
rm-suffix = $(1)
endif

define module_template
$(call gen-target,$(1)): $(addprefix $($(2)-dir)/,$(call getm-objs,$(1))) $($(1)-lds)
endef

define target_template
$(1)-dir ?= .$(1)
$(1)-objs := $(sort $(foreach m,$($(1)),$(call getm-objs,$(m))))
objs := $(obj) $$($(1)-objs)
$(1)-objs := $$(addprefix $$($(1)-dir)/,$$($(1)-objs))
endef

$(eval $(call target_template,host))
$(foreach m,$(host),$(eval $(call module_template,$(m),host)))

$(eval $(call target_template,progs))
$(foreach m,$(progs),$(eval $(call module_template,$(m),progs)))


deps-dir ?= .deps
deps := $(addprefix $(deps-dir)/,$(patsubst %.o,%.d,$(objs)))

quiet_cmd_host-objs = HOSTCC  $(notdir $@)
      cmd_host-objs = mkdir -p $(dir $@); $(HOSTCC) $(HOSTCFLAGS) -c -o $@ $<
quiet_cmd_host      = HOSTLD  $@
      cmd_host	    = mkdir -p $(dir $@); \
                      $(HOSTCC) $(HOSTLDFLAGS) -o $@ \
                      $(addprefix $(host-dir)/, \
                      $(call getm-objs,$(call rm-suffix,$(@F)))) \
                      $($(call rm-suffix,$(@F))-libs) $(HOSTLDLIBS) \
                      $($(call rm-suffix,$(@F))-lds)

quiet_cmd_progs-objs = CC      $(notdir $@)
      cmd_progs-objs =  $(CC) $(CFLAGS) -c -o $@ $<
quiet_cmd_progs      = LD      $@
      cmd_progs	     = mkdir -p $(dir $@); \
                      $(CC) $(LDFLAGS) -o $@ \
                      $(addprefix $(progs-dir)/, \
                         $(call getm-objs,$(call rm-suffix,$(@F)))) \
                      $($(call rm-suffix,$(@F))-lds) $(LDLIBS) \
                      $($(call rm-suffix,$(@F))-libs)

clean_dir = (cd $(1) && $(RM) $(2) -f) >/dev/null 2>&1; \
            $(RMDIR) $(1) >/dev/null 2>&1; :

quiet_cmd_clean_host = CLEAN   host
      cmd_clean_host = $(call clean_dir,$(host-dir),*.o);   \
                       $(RM) $(host-target) -f ; : 

quiet_cmd_clean_progs = CLEAN   progs
      cmd_clean_progs = $(call clean_dir,$(progs-dir),*.o); \
                        $(RM) $(progs-target) -f ; :

quiet_cmd_clean_dep = CLEAN   deps
      cmd_clean_dep = $(call clean_dir,$(deps-dir),*.d)

host-target := $(call gen-targets,$(host))
progs-target := $(call gen-targets,$(progs))

ifeq ($(Q),)
	QUIET_CC       = @echo '  CC      $@'; $(CC) $(CFLAGS) -c -o $@ $<
	QUIET_AR       = @echo '  AR      $@'; $(AR) $(ARFLAGS) $@ $^
	QUIET_LINK     = @echo '  LINK    $@'; $(CC)
	QUIET_RM       = @echo '  CLEAN   $1'; $(RM) -f $1
else
	QUIET_CC       = $(CC) $(CFLAGS) -c -o $@ $<
	QUIET_AR       = $(AR) $(ARFLAGS) $@ $^
	QUIET_LINK     = $(CC)
	QUIET_RM       = $(RM) -f
endif

.PHONY: all

all: DEPS $(host-target) $(progs-target)

$(host): 
	$(call cmd,host)

$(host-objs) : $(host-dir)/%.o : %.c
	@mkdir -p $(dir $@);
	$(call cmd,host-objs)

$(progs-target):
	$(call cmd,progs)

$(progs-objs) : $(progs-dir)/%.o : %.c
	@mkdir -p $(dir $@);
	$(call cmd,progs-objs)

DEPS: $(deps)
#$(deps)
$(deps-dir)/%.d:%.c
	@mkdir -p $(deps-dir)
	@$(CC) $(CFLAGS) -MM -MF $@.$$$$ $<; \
	sed 's,\($*\)\.o[ :]*,$(progs-dir)/\1.o $(host-dir)/\1.o $@ : Makefile ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean::
	$(call cmd,clean_host)
	$(call cmd,clean_progs)
	$(call cmd,clean_dep)

distclean: clean

-include $(deps)
