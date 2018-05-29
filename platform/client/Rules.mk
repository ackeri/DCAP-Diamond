d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	diamondclient.cc)

OBJS-client += $(o)diamondclient.o
