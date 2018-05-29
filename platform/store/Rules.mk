d := $(dir $(lastword $(MAKEFILE_LIST)))

SRCS += $(addprefix $(d), \
	client.cc occstore.cc \
	pubstore.cc server.cc \
	sharedclient.cc)

PROTOS += $(addprefix $(d), \
	strong-proto.proto)

LIB-strong-store := $(o)occstore.o

OBJS-strong-store := $(LIB-message) $(LIB-strong-store) $(LIB-store-common) \
	$(LIB-store-backend) $(o)strong-proto.o $(o)server.o

OBJS-strong-client := $(OBJS-vr-client) $(LIB-udptransport) $(LIB-store-frontend) $(LIB-store-common) $(o)strong-proto.o $(o)shardclient.o $(o)client.o

OBJS-pubstore := $(o)pubstore.o

$(d)server: $(LIB-udptransport) $(OBJS-vr-replica) $(OBJS-strong-store) \
	$(OBJS-pubstore) $(LIB-store-backend)

BINS += $(d)server
