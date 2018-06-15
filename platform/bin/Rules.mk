d := $(dir $(lastword $(MAKEFILE_LIST)))

$(d)libdiamond.so: $(patsubst %.o,%-pic.o, $(OBJS-client))
LDFLAGS-$(d)libdiamond.so += -shared

BINS += $(d)libdiamond.so

$(info warning $(OBJS-strong-client))
$(d)frontend: $(OBJS-frontend) $(OBJS-strong-client) $(PROTOOBJS) 
#LDFLAGS-$(d)frontend += 

BINS += $(d)frontend
