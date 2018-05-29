d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	terminalClient.cc)

OBJS-terminalclient := $(o)terminalClient.o

