d := $(dir $(lastword $(MAKEFILE_LIST)))

$(d)libtapir: $(OBJS) $(PROTOOBJS)
LDFLAGS-$(d)libtapir += -shared

BINS += $(d)libtapir
