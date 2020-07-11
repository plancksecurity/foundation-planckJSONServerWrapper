# this ifeq is not optional, because otherwise local.conf would always be built

ifeq ($(wildcard local.conf),local.conf)
-include local.conf
endif

# default config

# these are source builds

PEP_WEBSERVER?=../../webserver
PEP_ADAPTER_LIBRARY=../../libpEpAdapter
PEP_ENGINE?=../../pEpEngine
LIBETPAN_FDIK?=../../libetpan
SEQOUIA_PGP?=../../sequoia
NETTLE?=../../nettle
BOOST_INCLUDE_PATH=../../boost_1_72_0
BOOST_LIBRARY_PATH=../../boost_1_72_0/stage/lib

# path to installed libraries

PREFIX=$(HOME)

PEP_INCLUDE?=$(PREFIX)/include

LIBRARY_PATH?=/opt/local/lib

GMP_LIBRARY_PATH?=$(LIBRARY_PATH)
SQLITE3_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBZ_LIBRARY_PATH?=$(LIBRARY_PATH)
ICONV_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBEVENT_LIBRARY_PATH=$(LIBRARY_PATH)

AR?=ar
CC?=cc
CXX?=c++
CFLAGS+=-std=c14
CXXFLAGS+=-I$(BOOST_INCLUDE_PATH) -I$(PEP_INCLUDE) -std=c++14
LDFLAGS+=-std=c++14

ifdef NDEBUG
	CFLAGS+=-O3 -DNDEBUG -fvisibility=hidden
	CXXFLAGS+=-O3 -DNDEBUG -fvisibility=hidden
else
	CFLAGS+=-O0 -g -fvisibility=hidden
	CXXFLAGS+=-O0 -g -fvisibility=hidden
endif

TARGET=pEp-mini-json-adapter
LIB_TARGET=libjson-adapter.a

ALL_SOURCE=$(filter-out unittest_%.cc,$(wildcard *.cc))
LIB_SOURCE=$(filter-out miniadapter-%.cc,$(ALL_SOURCE))
MA_SOURCE=$(wildcard miniadapter-*.cc)
DEPENDS=$(subst .cc,.d,$(ALL_SOURCE))
ALL_OBJECTS=$(subst .d,.o,$(DEPENDS))
LIB_OBJECTS=$(subst .cc,.o,$(LIB_SOURCE))
MA_OBJECTS=$(subst .cc,.o,$(MA_SOURCE))
SPIRIT_SOURCE=$(wildcard json_spirit/*.cpp)
SPIRIT_OBJECTS=$(subst .cpp,.o,$(SPIRIT_SOURCE))

TEST_SOURCE=$(wildcard test_*.cc)
TEST_OBJECTS=$(subst .cc,.o,$(TEST_SOURCE))
TESTS=$(subst .cc,,$(TEST_SOURCE))

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

$(LIB_TARGET): $(LIB_OBJECTS)
	$(AR) -cr $@ $^

$(TARGET): $(LIB_TARGET) $(MA_OBJECTS) $(SPIRIT_OBJECTS)
	$(CXX) $(LDFLAGS) -liconv \
		-lobjc -framework Foundation -framework Security \
		$(PEP_WEBSERVER)/libpEpwebserver.a \
		$(BOOST_LIBRARY_PATH)/libboost_filesystem.a \
		$(BOOST_LIBRARY_PATH)/libboost_program_options.a \
		$(BOOST_LIBRARY_PATH)/libboost_system.a \
		$(BOOST_LIBRARY_PATH)/libboost_thread.a \
		$(BOOST_LIBRARY_PATH)/libboost_regex.a \
		$(ICONV_LIBRARY_PATH)/libiconv.a \
		$(LIBZ_LIBRARY_PATH)/libz.a \
		$(SQLITE3_LIBRARY_PATH)/libsqlite3.a \
		$(GMP_LIBRARY_PATH)/libgmp.a \
		$(NETTLE)/libnettle.a $(NETTLE)/libhogweed.a \
		$(SEQOUIA_PGP)/target/release/libsequoia_ffi.a \
		$(SEQOUIA_PGP)/target/release/libsequoia_openpgp_ffi.a \
		$(LIBETPAN_FDIK)/src/.libs/libetpan.a \
		$(PEP_ENGINE)/src/libpEpEngine.a $(PEP_ENGINE)/asn.1/libasn1.a \
		$(PEP_ADAPTER_LIBRARY)/libpEpAdapter.a \
		$(MA_OBJECTS) $(SPIRIT_OBJECTS) $(LIB_TARGET) -o "$@"

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

test_%: test_%.o $(TARGET)
	$(CXX) $(LDFLAGS) $(TARGET) -o $@ $<

test: $(TESTS)
	for i in $(TESTS) ; do ./$$i ; done

.PHONY: clean uninstall install

install: $(TARGET)
	mkdir -p $(PREFIX)/include/pEp
	cp -v *.hh $(PREFIX)/include/pEp
	cp -v lib*.a $(PREFIX)/lib/

uninstall:
	for i in *.hh; do rm -f $(PREFIX)/include/pEp/\$i ; done

clean:
	rm -vf *.o *.d *.d.* $(TARGET) $(TESTS)

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
	@echo "SEQOUIA_PGP=$(SEQOUIA_PGP)" >> local.conf
	@echo >> local.conf
	@echo "# path to nettle and hogweed crypto libraries" >> local.conf
	@echo >> local.conf
	@echo "NETTLE=$(NETTLE)" >> local.conf
	@echo >> local.conf
	@echo "# path where to find boost" >> local.conf
	@echo >> local.conf
	@echo "BOOST_INCLUDE=$(BOOST_INCLUDE_PATH)" >> local.conf
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
	@echo >> local.conf
	@echo "# path where to find libevent" >> local.conf
	@echo >> local.conf
	@echo "LIBEVENT_LIBRARY_PATH=$(LIBEVENT_LIBRARY_PATH)" >> local.conf

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
