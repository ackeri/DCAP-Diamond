d := $(dir $(lastword $(MAKEFILE_LIST)))
o := .obj/$(d)

SRCS += $(addprefix $(d), \
	boolean.cc booleanlist.cc counter.cc \
	id.cc list.cc long.cc object.cc set.cc \
	string.cc stringlist.cc stringqueue.cc \
	stringset.cc)

OBJS += $(addprefix $(o), \
	boolean.o booleanlist.o counter.o \
	id.o list.o long.o object.o set.o \
	string.o stringlist.o stringqueue.o \
	stringset.o)

