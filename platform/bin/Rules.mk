d := $(dir $(lastword $(MAKEFILE_LIST)))

$(d)libdiamond.so: $(patsubst %.o,%-pic.o, $(OBJS-client))
LDFLAGS-$(d)libdiamond.so += -shared

BINS += $(d)libdiamond.so

$(d)frontend: $(OBJS-frontend) 
#LDFLAGS-$(d)frontend += 

BINS += $(d)frontend
