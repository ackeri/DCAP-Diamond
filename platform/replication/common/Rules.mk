d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	client.cc configuration.cc \
	log.cc replica.cc)

PROTOS += $(addprefix $(d), \
	    request.proto)

LIB-request := $(o)request.o

OBJS-client := $(o)client.o \
               $(LIB-message) $(LIB-configuration) \
               $(LIB-transport) $(LIB-request)

OBJS-replica := $(o)replica.o $(o)log.o $(o)configuration.o \
                $(LIB-message) $(LIB-request) \
                $(LIB-configuration) $(LIB-udptransport)

