# this ifeq is not optional, because otherwise local.conf would always be built

ifeq ($(wildcard local.conf),local.conf)
-include local.conf
endif

# default config

# set to 'etpan' or 'pEpMIME'
MIME_LIB?=etpan

PREFIX?=$(HOME)/local

# where pEp stuff is installed into:
PEP_LIB_PATH?=$(PREFIX)/lib
PEP_INCLUDE?=$(PREFIX)/include

# where libs from the packages systems are installed into:
LIBRARY_PATH?=/usr/local/lib

CFLAGS+=-std=c14 -fvisibility=hidden
CXXFLAGS+= -I$(PEP_INCLUDE) -std=c++14 -fvisibility=hidden
LDFLAGS+=-std=c++14 -L$(PEP_LIB_PATH) -L$(LIBRARY_PATH)
LDLIBS+= -lpEpEngine -lpEpAdapter -lpEpwebserver -lasn1 \
	 -lsequoia_openpgp_ffi -ldl -lhogweed -lnettle -lgmp \
	 -lz

# Boost naming postfix
BOOST_MT?=-mt

# This idiom is from the pEp Engine Makefile. Don't ask... ;-)
BUILD_ON:=$(shell uname)
BUILD_FOR=$(BUILD_ON)
ifeq ($(BUILD_ON),Darwin)
    ifeq ($(BUILD_FOR),Darwin)
        LDLIBS+= -liconv -lobjc -framework Foundation -framework Security
        LDLIBS+= -lboost_filesystem$(BOOST_MT) -lboost_program_options$(BOOST_MT) -lboost_system$(BOOST_MT) -lboost_thread$(BOOST_MT) -lboost_regex$(BOOST_MT)
    else
        $(error I do not know how to make for $(BUILD_FOR) on $(BUILD_ON))
    endif
else ifeq ($(BUILD_ON),Linux)
    ifeq ($(BUILD_FOR),Linux)
        LDLIBS+= -luuid \
         -lboost_filesystem -lboost_program_options -lboost_system -lboost_thread -lboost_regex -lsqlite3 -ldl

    else
        $(error I do not know how to make for $(BUILD_FOR) on $(BUILD_ON))
    endif
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
    LDLIBS+=-letpan -lz
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

.PHONY: all
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
	$(CXX) $(LDFLAGS) \
		$(MA_OBJECTS) $(SPIRIT_OBJECTS)  $(LIB_TARGET) \
		-o "$@" $(LDLIBS)


unittests: $(TEST_OBJECTS) $(LIB_TARGET) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lgtest -lgtest_main

parse_mail: parse_mail.o nfc.o $(LIB_TARGET) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: test clean uninstall install

test: unittests
	./unittests

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

local.conf:
	@echo "# these are installation directories" >> local.conf
	@echo "# ----------------------------------" >> local.conf
	@echo >> local.conf
	@echo "# pEp installation header file directory" >> local.conf
	@echo "PEP_INCLUDE=$(PEP_INCLUDE)" >> local.conf
	@echo >> local.conf
	@echo "# installation destination for make install" >> local.conf
	@echo "PREFIX?=$(PREFIX)" >> local.conf
	@echo >> local.conf
	@echo "# Additional options" >> local.conf
	@echo "# ------------------" >> local.conf
	@echo >> local.conf
	@echo "# for a dynamically linked binary (for development) remove this." >> local.conf
	@echo "# also add -L<path> here if .a or .so files from non-standard paths are needed." >> local.conf
	@echo "# If you are using clang, it's \"-static\", but that will fail if you did not compile a static version of libcrt0 and libc[++] yourself." >> local.conf
	@echo "LDFLAGS+=--static" >> local.conf
	@echo >> local.conf
	@echo "# Building BOOST yourself, the artefact names are not postfixed by deafult" >> local.conf
	@echo "# E.g. lib is named \"libboost_regex.a\", not \"libboost_regex-mt.a\"." >> local.conf
	@echo "# Set to empty string if this is the case for you." >> local.conf
	@echo "BOOST_MT=\"-mt\"" >> local.conf
	@echo >> local.conf
	@echo "# Path where pEp libraries are installed." >> local.conf
	@echo "PEP_LIB_PATH=$(PEP_LIB_PATH)" >> local.conf
	@echo >> local.conf
	@echo "# Path where system libraries are installed." >> local.conf
	@echo "LIBRARY_PATH=$(LIBRARY_PATH)" >> local.conf
	@echo >> local.conf
	@echo "# remove this for a debug build." >> local.conf
	@echo "NDEBUG=yes" >> local.conf

help:
	@echo "usage:"
	@echo "    make local.conf # optional"
	@echo "        generate a local.conf with default paths"
	@echo
	@echo "    make libjson-adapter.a"
	@echo "        build adapter library"
	@echo
	@echo "    make pEp-mini-json-adapter # default"
	@echo "       compile and link mini adapter"
	@echo
