d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	client.cc server.cc)

OBJS-frontend := $(o)client.o $(o)server.o

