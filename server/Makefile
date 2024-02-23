# this ifeq is not optional, because otherwise local.conf would always be built

ifeq ($(wildcard local.conf),local.conf)
-include local.conf
endif

# default config

# set to 'etpan' or 'pEpMIME'
MIME_LIB?=etpan
PREFIX?=$(HOME)/local

# pEp Dependencies
PEP_INC_PATH?=$(PREFIX)/include
PEP_LIB_PATH?=$(PREFIX)/lib

# System Dependencies
SYS_INC_PATH?=/usr/local/include
SYS_LIB_PATH?=/usr/local/lib

CFLAGS+=-std=c14 -fvisibility=hidden
CXXFLAGS+=-I$(SYS_INC_PATH) -I$(PEP_INC_PATH) -std=c++14 -fvisibility=hidden
LDFLAGS+=-std=c++14 -L$(PEP_LIB_PATH) -L$(SYS_LIB_PATH)
LDLIBS+= -lasn1 -lpEpEngine -lpep_engine_sequoia_backend -lbotan-2 -lpEpAdapter -lpEpwebserver -lpEpCxx11 -lstdc++

# Boost naming postfix
BOOST_MT?=-mt

# This idiom is from the pEp Engine Makefile. Don't ask... ;-)
BUILD_ON:=$(shell uname)
BUILD_FOR=$(BUILD_ON)
ifeq ($(BUILD_ON),Darwin)
	LDLIBS+= -lobjc -framework Foundation -framework Security
	LDLIBS+= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libiconv.tbd
	LDLIBS+= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/lib/libz.tbd
	LDLIBS+= /opt/homebrew/lib/libboost_filesystem-mt.a
	LDLIBS+= /opt/homebrew/lib/libboost_program_options-mt.a
	LDLIBS+= /opt/homebrew/lib/libboost_system-mt.a
	LDLIBS+= /opt/homebrew/lib/libboost_thread-mt.a
	LDLIBS+= /opt/homebrew/lib/libboost_regex-mt.a

	# ASAN is not strictly a MacOS/clang feature, but won't work with musl.
	CXXFLAGS+= -fsanitize=address
	LDFLAGS+=  -fsanitize=address
else ifeq ($(BUILD_ON),Linux)
	LDLIBS+= -luuid -lboost_filesystem -lboost_program_options -lboost_system -lboost_thread -lboost_regex -lpthread -ldl
	LDLIBS+= -lz
else
    $(error I do not know how to make for $(BUILD_FOR) on $(BUILD_ON))
endif

# DEBUG=0 -> release build
# DEBUG=1 or undefined -> debug build
ifeq ($(DEBUG),0)
	CFLAGS+=-O3 -DNDEBUG  
	CXXFLAGS+=-O3 -DNDEBUG
else
	CFLAGS+=-O0 -g -DDEBUG_ENABLED
	CXXFLAGS+=-O0 -g -DDEBUG_ENABLED
endif


TARGET=pEp-mini-json-adapter
LIB_TARGET=libjson-adapter.a

ifeq ($(MIME_LIB),etpan)
    LDLIBS+=-letpan
    OMIT_NFC_SOURCE=
else ifeq ($(MIME_LIB),pEpMIME)
    LDLIBS+=-lpEpMIME
    
    # is already in libpEpMIME:
    OMIT_NFC_SOURCE=nfc.cc nfc_sets.cc
else
    $(error "You must set MIME_LIB to 'etpan' or 'pEpMIME'!" )
endif


ALL_SOURCE=$(filter-out parse_mail.cc unittest_%.cc servertest.cc, $(wildcard *.cc))
MA_SOURCE=$(wildcard mini-adapter-*.cc)

LIB_SOURCE=$(filter-out $(MA_SOURCE) $(OMIT_NFC_SOURCE), $(ALL_SOURCE))

DEPENDS=$(subst .cc,.d,$(ALL_SOURCE))

ALL_OBJECTS=$(subst .d,.o,$(DEPENDS))
LIB_OBJECTS=$(subst .cc,.o,$(LIB_SOURCE))
MA_OBJECTS=$(subst .cc,.o,$(MA_SOURCE))

SPIRIT_SOURCE=$(wildcard json_spirit/*.cc)
SPIRIT_OBJECTS=$(subst .cc,.o,$(SPIRIT_SOURCE))

TEST_SOURCE=$(wildcard unittest_*.cc)
TEST_OBJECTS=$(subst .cc,.o,$(TEST_SOURCE))


.PHONY: all test clean uninstall install

all: $(TARGET)

%.d: %.cc
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# If only the goal 'clean' is given, do not generate and include the '%.d' files.
ifneq ($(MAKECMDGOALS),clean)
	-include $(DEPENDS)
endif

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^


$(LIB_TARGET): $(LIB_OBJECTS)
	$(AR) -cr $@ $^

$(TARGET): $(LIB_TARGET) $(MA_OBJECTS) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) $(MA_OBJECTS) $(SPIRIT_OBJECTS)  $(LIB_TARGET) -o "$@" $(LDLIBS)

unittests: $(TEST_OBJECTS) $(LIB_TARGET) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

parse_mail: parse_mail.o nfc.o $(LIB_TARGET) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test: unittests
	HOME=.; ./unittests

install: $(TARGET)
	mkdir -p $(DESTDIR)$(PREFIX)/include/pEp
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -v *.hh $(DESTDIR)$(PREFIX)/include/pEp
	cp -v lib*.a $(DESTDIR)$(PREFIX)/lib/
	cp -v $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(addprefix $(DESTDIR)$(PREFIX)/include/pEp/,$(wildcard *.hh))

clean:
	rm -vf *.a *.o */*.o *.d *.d.* $(TARGET) $(TESTS) encode_decode
