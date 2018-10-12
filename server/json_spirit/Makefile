.PHONY: all clear

CXXFLAGS = -Wall -O3 -std=c++11 -g
LDFLAGS  = -lpthread
HEADERS = $(wildcard *.h)

all: libjson_spirit.a


libjson_spirit.a: json_spirit_reader.o json_spirit_value.o json_spirit_writer.o
	rm -f $@
	ar rs $@ $^


%.o : %.cpp $(HEADERS)
	c++ $(CXXFLAGS) -c $<


clean:
	rm -vf *.a
	rm -vf *.o

