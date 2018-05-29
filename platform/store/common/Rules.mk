d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	promise.cc tracer.cc transaction.cc \
	version.cc timestamp.cc truetime.cc)

PROTOS += $(addprefix $(d), \
	common-proto.proto notification-proto.proto)

LIB-store-common := $(o)common-proto.o $(o)promise.o $(o)timestamp.o \
					$(o)tracer.o $(o)transaction.o $(o)truetime.o \
					$(o)version.o


