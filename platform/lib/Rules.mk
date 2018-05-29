d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	configuration.cc latency.cc \
	lookup3.cc memory.cc message.cc \
	simtransport.cc tcptransport.cc \
	transport.cc udptransport.cc)

LIB-hash := $(o)lookup3.o

LIB-message := $(o)message.o $(LIB-hash)

LIB-hashtable := $(LIB-hash) $(LIB-message)

LIB-memory := $(o)memory.o

LIB-latency := $(o)latency.o $(o)latency-format.o $(LIB-message)

LIB-configuration := $(o)configuration.o $(LIB-message)

LIB-transport := $(o)transport.o $(LIB-message) $(LIB-configuration)

LIB-simtransport := $(o)simtransport.o $(LIB-transport)

LIB-repltransport := $(o)repltransport.o $(LIB-transport)

LIB-udptransport := $(o)udptransport.o $(LIB-transport)

LIB-tcptransport := $(o)tcptransport.o $(LIB-transport)

LIB-persistent_register := $(o)persistent_register.o $(LIB-message)


