BUILD_ON=$(shell uname)
BUILD_FOR=$(BUILD_ON)

CXX = c++ -std=c++11 -Wall -O0 -g -fstack-protector-all

ifeq ($(BUILD_ON),Darwin)
ifeq ($(BUILD_FOR),Darwin)

CXXFLAGS = -I/opt/local/include -I$(HOME)/include -Ijson_spirit
LDFLAGS  = -L/opt/local/lib -L$(HOME)/lib -lpthread -levent -lpEpEngine -luuid -lgpgme-pthread -letpan -lboost_system-mt -lboost_filesystem-mt
endif

else

CXXFLAGS = -I$(HOME)/include -I$(HOME)/local/include/ -Ijson_spirit
LDFLAGS  = -L$(HOME)/lib -L$(HOME)/local/lib -lpthread -levent -lpEpEngine -luuid -lgpgme-pthread -letpan -lboost_system -lboost_filesystem -lboost_program_options -lboost_thread

endif

.PHONY: all clear

all: mt-server servertest

mt-server: main.o libjson-adapter.a
	$(CXX) $(CXXFLAGS)  -o $@ $^ ../../pEpEngine/asn.1/libasn1.a  $(LDFLAGS)

servertest: servertest.o 
	$(CXX) $(CXXFLAGS)  -o $@ $^   $(LDFLAGS)


libjson-adapter.a: \
    json_spirit/json_spirit_reader.o json_spirit/json_spirit_value.o json_spirit/json_spirit_writer.o \
    json-adapter.o registry.o nfc.o json_rpc.o \
    function_map.o pep-types.o \
    security-token.o \
    nfc_sets.o base64.o \
    nulllogger.o \
    daemonize.o \
	ar rcs $@ $^


%.o : %.cc  %.hh  Makefile
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -vf mt-server servertest
	rm -vf *.o
	rm -vf json_spirit/*.o
	rm -vf *.a

