BUILD_ON=$(shell uname)
BUILD_FOR=$(BUILD_ON)

ifeq ($(BUILD_ON),Darwin)
ifeq ($(BUILD_FOR),Darwin)

CXXFLAGS = -Wall -O0 -std=c++11 -g -I/opt/local/include -I$(HOME)/include -Ijson_spirit -fstack-protector-all
LDFLAGS  = -L/opt/local/lib -L$(HOME)/lib -lpthread -levent -lpEpEngine -lgpgme-pthread -letpan -lboost_system-mt -lboost_filesystem-mt
endif

else

CXXFLAGS = -Wall -O0 -std=c++11 -g -I/home/deb/local/include/ -Ijson_spirit -fstack-protector-all
LDFLAGS  = -L/home/deb/local/lib -lpthread -levent -lpEpEngine -lgpgme-pthread -letpan -lboost_system -lboost_filesystem

endif

.PHONY: all clear

all: mt-server

mt-server: mt-server.o registry.o nfc.o json_rpc.o function_map.o pep-types.o \
    security-token.o \
    json_spirit/json_spirit_reader.o json_spirit/json_spirit_value.o json_spirit/json_spirit_writer.o 
	c++ $(CXXFLAGS)  -o $@ $^ $(LDFLAGS) ../../pEpEngine/asn.1/libasn1.a


%.o : %.cc %.hh
	c++ $(CXXFLAGS) -c $<

clean:
	rm -vf mt-server
	rm -vf *.o
