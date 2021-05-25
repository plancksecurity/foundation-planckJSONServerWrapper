# this ifeq is not optional, because otherwise local.conf would always be built

ifeq ($(wildcard local.conf),local.conf)
-include local.conf
endif

# default config

# these are source builds

PEP_WEBSERVER?=../../webserver
PEP_ADAPTER_LIBRARY?=../../libpEpAdapter
PEP_ENGINE?=../../pEpEngine/src
LIBETPAN_FDIK?=../../libetpan/src/.libs
SEQUOIA_PGP?=../../sequoia/target/release
NETTLE?=../../nettle
BOOST_INCLUDE_PATH?=../../boost_1_72_0
BOOST_LIBRARY_PATH?=../../boost_1_72_0/stage/lib
ASN1_PATH?=$(PEP_ENGINE)/asn.1

# path to installed libraries

PREFIX?=$(HOME)

PEP_INCLUDE?=$(PREFIX)/include

LIBRARY_PATH?=$(PREFIX)/lib

GMP_LIBRARY_PATH?=$(LIBRARY_PATH)
SQLITE3_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBZ_LIBRARY_PATH?=$(LIBRARY_PATH)
ICONV_LIBRARY_PATH?=$(LIBRARY_PATH)
LIBEVENT_LIBRARY_PATH?=$(LIBRARY_PATH)

AR?=ar
CC?=cc
CXX?=c++
CFLAGS+=-std=c14
CXXFLAGS+=-I$(BOOST_INCLUDE_PATH) -I$(PEP_INCLUDE) -std=c++14
LDFLAGS+=-std=c++14

ifdef NDEBUG
	CFLAGS+=-O3 -DNDEBUG -fvisibility=hidden -fno-stack-check
	CXXFLAGS+=-O3 -DNDEBUG -fvisibility=hidden -fno-stack-check
else
	CFLAGS+=-O0 -g -DDEBUG_ENABLED -fvisibility=hidden -fno-stack-check
	CXXFLAGS+=-O0 -g -DDEBUG_ENABLED -fvisibility=hidden -fno-stack-check
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

# Filename of the unittest binary
TARGET_GTEST=unittests


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
	$(CXX) $(LDFLAGS) -lobjc -framework Foundation -framework Security \
		$(PEP_WEBSERVER)/libpEpwebserver.a \
		$(BOOST_LIBRARY_PATH)/libboost_filesystem$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_program_options$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_system$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_thread$(BOOST_MT).a \
		$(BOOST_LIBRARY_PATH)/libboost_regex$(BOOST_MT).a \
		$(ICONV_LIBRARY_PATH)/libiconv.a \
		$(LIBZ_LIBRARY_PATH)/libz.a \
		$(SQLITE3_LIBRARY_PATH)/libsqlite3.a \
		$(GMP_LIBRARY_PATH)/libgmp.a \
		$(NETTLE)/libnettle.a $(NETTLE)/libhogweed.a \
		$(SEQUOIA_PGP)/libsequoia_ffi.a \
		$(SEQUOIA_PGP)/libsequoia_openpgp_ffi.a \
		$(LIBETPAN_FDIK)/libetpan.a \
		$(PEP_ENGINE)/libpEpEngine.a \
		$(ASN1_PATH)/libasn1.a \
		$(PEP_ADAPTER_LIBRARY)/libpEpAdapter.a \
		$(MA_OBJECTS) $(SPIRIT_OBJECTS) $(LIB_TARGET) -o "$@"

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -o $@ -c $<

gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -o $@ -c $<

$(TARGET_GTEST): unittest_json.o unittest_nfc.o unittest_decompose.o unittest_rpc.o \
    unittest_logger.o unittest_pEp.o unittest_string.o \
    gtest-all.o gtest_main.o libjson-adapter.a
	$(CXX) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@


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
