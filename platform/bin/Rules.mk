d := $(dir $(lastword $(MAKEFILE_LIST)))

$(d)libdiamond: $(OBJS) $(PROTOOBJS)
LDFLAGS-$(d)libdiamond += -shared

$(d)frontend: $(OBJS) $(PROTOOBJS)
#LDFLAGS-$(d)frontend += 

BINS += $(d)libdiamond $(d)frontend
