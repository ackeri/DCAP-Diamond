d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	asynccacheclient.cc cacheclient.cc)

LIB-store-frontend := $(o)bufferclient.o

