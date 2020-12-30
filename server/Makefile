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


# paths to the .a libraries. Can be overridden in local.conf
PEP_WEBSERVER?=$(PEP_LIB_PATH)
PEP_ADAPTER_LIBRARY=$(PEP_LIB_PATH)
PEP_ENGINE?=$(PEP_LIB_PATH)
LIBETPAN_FDIK?=$(PEP_LIB_PATH)
PEP_MIME_PATH?=$(PEP_LIB_PATH)
SEQUOIA_PGP?=$(PEP_LIB_PATH)
ASN1_PATH?=$(PEP_LIB_PATH)
NETTLE?=$(LIBRARY_PATH)

BOOST_INCLUDE_PATH?=/usr/local/include
BOOST_LIBRARY_PATH?=$(LIBRARY_PATH)


LIBRARY_PATH?=$(PREFIX)/lib

GMP_LIBRARY_PATH?=$(LIBRARY_PATH)
SQLITE3_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBZ_LIBRARY_PATH?=$(LIBRARY_PATH)
ICONV_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBEVENT_LIBRARY_PATH=$(LIBRARY_PATH)

AR?=/usr/bin/ar
CC?=cc
CXX?=c++
CFLAGS+=-std=c14 -fvisibility=hidden
CXXFLAGS+=-I$(BOOST_INCLUDE_PATH) -I$(PEP_INCLUDE) -std=c++14 -fvisibility=hidden
LDFLAGS+=-std=c++14

ifdef NDEBUG
	CFLAGS+=-O3 -DNDEBUG  
	CXXFLAGS+=-O3 -DNDEBUG
else
	CFLAGS+=-O0 -g -DDEBUG_ENABLED
	CXXFLAGS+=-O0 -g -DDEBUG_ENABLED
endif

TARGET=pEp-mini-json-adapter
LIB_TARGET=libjson-adapter.a

ALL_SOURCE=$(filter-out parse_mail.cc unittest_%.cc,$(wildcard *.cc))
LIB_SOURCE=$(filter-out mini-adapter-%.cc,$(ALL_SOURCE))
MA_SOURCE=$(wildcard mini-adapter-*.cc)
DEPENDS=$(subst .cc,.d,$(ALL_SOURCE))
ALL_OBJECTS=$(subst .d,.o,$(DEPENDS))
LIB_OBJECTS=$(subst .cc,.o,$(LIB_SOURCE))
MA_OBJECTS=$(subst .cc,.o,$(MA_SOURCE))
SPIRIT_SOURCE=$(wildcard json_spirit/*.cpp)
SPIRIT_OBJECTS=$(subst .cpp,.o,$(SPIRIT_SOURCE))

TEST_SOURCE=$(wildcard unittest_*.cc)
TEST_OBJECTS=$(subst .cc,.o,$(TEST_SOURCE))

ifeq ($(MIME_LIB),etpan)
    LDLIBS+=-letpan
    MIME_LIB_A=$(LIBETPAN_FDIK)/libetpan.a
else ifeq ($(MIME_LIB),pEpMIME)
    LDLUBS+=-lpEpMIME
    MIME_LIB_A=$(PEP_MIME_PATH)/libpEpMIME.a
else
    $(error "You must set MIME_LIB to 'etpan' or 'pEpMIME'!" )
endif


all: $(TARGET) unittests parse_mail

%.d: %.cc
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# If only the goal 'clean' is given, do not generate and include the '%.d' files.
ifneq ($(MAKECMDGOALS),clean)
	-include $(DEPENDS)
endif

$(LIB_TARGET): $(LIB_OBJECTS)
	$(AR) -cr $@ $^

$(TARGET): $(LIB_TARGET) $(MA_OBJECTS) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) \
		$(MA_OBJECTS) $(SPIRIT_OBJECTS)  $(LIB_TARGET) \
		-lobjc -framework Foundation -framework Security \
		$(PEP_ENGINE)/libpEpEngine.a \
		$(PEP_ADAPTER_LIBRARY)/libpEpAdapter.a \
		$(PEP_WEBSERVER)/libpEpwebserver.a \
		$(BOOST_LIBRARY_PATH)/libboost_filesystem$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_program_options$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_system$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_thread$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_regex$(BOOST_MT).a \
		$(SEQUOIA_PGP)/libsequoia_openpgp_ffi.a \
		$(MIME_LIB_A) \
		$(ASN1_PATH)/libasn1.a \
		$(LIBZ_LIBRARY_PATH)/libz.a \
		$(SQLITE3_LIBRARY_PATH)/libsqlite3.a \
		$(NETTLE)/libhogweed.a \
		$(NETTLE)/libnettle.a \
		$(GMP_LIBRARY_PATH)/libgmp.a \
		$(ICONV_LIBRARY_PATH)/libiconv.a \
		-o "$@"

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

unittests: $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^

parse_mail: parse_mail.o
	$(CXX) $(CXXFLAGS) -o $@ $^

.PHONY: clean uninstall install

install: $(TARGET)
	mkdir -p $(PREFIX)/include/pEp
	cp -v *.hh $(PREFIX)/include/pEp
	cp -v lib*.a $(PREFIX)/lib/

uninstall:
	for i in *.hh; do rm -f $(PREFIX)/include/pEp/\$i ; done

clean:
	rm -vf *.o *.d *.d.* $(TARGET) $(TESTS) encode_decode

local.conf:
	@echo "# the following are directories with the source build" >> local.conf
	@echo "# ---------------------------------------------------" >> local.conf
	@echo >> local.conf
	@echo "# path to pEp webserver" >> local.conf
	@echo >> local.conf
	@echo "PEP_WEBSERVER=$(PEP_WEBSERVER)" >> local.conf
	@echo >> local.conf
	@echo "# path to pEp adapter library" >> local.conf
	@echo >> local.conf
	@echo "PEP_ADAPTER_LIBRARY=$(PEP_ADAPTER_LIBRARY)" >> local.conf
	@echo >> local.conf
	@echo "# path to pEp engine" >> local.conf
	@echo >> local.conf
	@echo "PEP_ENGINE=$(PEP_ENGINE)" >> local.conf
	@echo >> local.conf
	@echo "# path to fdik fork of libetpan" >> local.conf
	@echo >> local.conf
	@echo "LIBETPAN_FDIK=$(LIBETPAN_FDIK)" >> local.conf
	@echo >> local.conf
	@echo "# path to Sequoia PGP" >> local.conf
	@echo >> local.conf
	@echo "SEQUOIA_PGP=$(SEQUOIA_PGP)" >> local.conf
	@echo >> local.conf
	@echo "# path to nettle and hogweed crypto libraries" >> local.conf
	@echo >> local.conf
	@echo "NETTLE=$(NETTLE)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find boost" >> local.conf
	@echo >> local.conf
	@echo "BOOST_INCLUDE_PATH=$(BOOST_INCLUDE_PATH)" >> local.conf
	@echo "BOOST_LIBRARY_PATH=$(BOOST_LIBRARY_PATH)" >> local.conf
	@echo >> local.conf
	@echo >> local.conf
	@echo "# these are installation directories" >> local.conf
	@echo "# ----------------------------------" >> local.conf
	@echo >> local.conf
	@echo "# pEp installation header file directory" >> local.conf
	@echo >> local.conf
	@echo "PEP_INCLUDE=$(PEP_INCLUDE)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find GMP" >> local.conf
	@echo >> local.conf
	@echo "GMP_LIBRARY_PATH=$(GMP_LIBRARY_PATH)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find SQLite3" >> local.conf
	@echo >> local.conf
	@echo "SQLITE3_LIBRARY_PATH=$(SQLITE3_LIBRARY_PATH)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find libz" >> local.conf
	@echo >> local.conf
	@echo "LIBZ_LIBRARY_PATH=$(LIBZ_LIBRARY_PATH)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find GNU iconv" >> local.conf
	@echo >> local.conf
	@echo "ICONV_LIBRARY_PATH=$(ICONV_LIBRARY_PATH)" >> local.conf

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
