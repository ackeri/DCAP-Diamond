d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	commstore.cc kvstore.cc \
	pubversionstore.cc versionstore.cc txnstore.cc)

LIB-store-backend := $(o)kvstore.o $(o)txnstore.o $(o)versionstore.o \
	$(o)commstore.o $(o)pubversionstore.o $(LIB-tcptransport) 
