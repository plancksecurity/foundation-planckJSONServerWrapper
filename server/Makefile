# Copyright 2017 pEp Security SA
#
# This file is part of pEp JSON Server Adapter.
#
# pEp JSON Server Adapter is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License, version 3, as
# published by the Free Software Foundation.
#
# pEp JSON Server Adapter is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with pEp JSON Server Adapter.  If not, see <http://www.gnu.org/licenses/>.


include Makefile.conf

ifneq ($(wildcard local.conf),)
    $(info ================================================)
    $(info Overrides in \`local.conf\` are used.)
    $(info ================================================)
endif

ifdef BUILD_CONFIG
    $(info ================================================)
    $(info Overrides in \`$(BUILD_CONFIG)\` are used.)
    $(info ================================================)
endif

# These source files shall _not_ be part of the libjson-adapter.a:
NO_SOURCE=$(wildcard mini-adapter-*.cc) servertest.cc $(wildcard unittest_*.cc)

ALL_SOURCE=$(filter-out $(NO_SOURCE),$(wildcard json_spirit/*.cpp))
ALL_SOURCE+= prefix-config.cc
ALL_SOURCE+= $(filter-out $(NO_SOURCE),$(wildcard *.cc))
DEPENDS0=$(subst .cpp,.d,$(ALL_SOURCE))
DEPENDS=$(subst .cc,.d,$(DEPENDS0))
ALL_OBJECTS0=$(subst .cpp,.o,$(ALL_SOURCE))
ALL_OBJECTS=$(subst .cc,.o,$(ALL_OBJECTS0))

ifeq ($(BUILD_FOR),Linux)
    LIBPATH=LD_LIBRARY_PATH
else
    LIBPATH=DYLD_LIBRARY_PATH
endif

ifeq ($(BUILD_FOR),Linux)
    LD_EAGER_VAR=LD_BIND_NOW=placeholder
else
    LD_EAGER_VAR=DYLD_BIND_AT_LAUNCH=placeholder
endif

# Create a list of the extra library paths for the loader.
# Note that += can not be used here, as it changes the amount of whitespace
EXTRA_LIB_PATHS=$(PREFIX)/lib:
ifdef BOOST_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(BOOST_LIB)):
endif
ifdef ENGINE_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(ENGINE_LIB)):
endif
ifdef ETPAN_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(ETPAN_LIB)):
endif
ifdef EVENT_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(EVENT_LIB)):
endif
ifdef GPGME_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(GPGME_LIB)):
endif
ifdef UUID_LIB
    EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS)$(patsubst -L%,%,$(UUID_LIB)):
endif
# Remove trailing ':'
EXTRA_LIB_PATHS:=$(EXTRA_LIB_PATHS::=)

CMD_PFX=$(LIBPATH)=$(EXTRA_LIB_PATHS)

ifdef DEBUG
    CMD_PFX+= $(LD_EAGER_VAR)
endif

.PHONY: all
all: $(TARGET) $(TARGET_TEST) $(TARGET_GTEST)

$(TARGET): mini-adapter-main.o mini-adapter-impl.o libjson-adapter.a
	$(CXX) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TARGET)-static: mini-adapter-main.o mini-adapter-impl.o libjson-adapter.a
	$(CXX) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -static -o $@

$(TARGET_TEST): servertest.o libjson-adapter.a
	$(CXX) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TARGET_GTEST): unittest_json.o unittest_nfc.o unittest_decompose.o unittest_rpc.o \
    unittest_logger.o unittest_pEp.o unittest_sha1.o \
    gtest-all.o gtest_main.o libjson-adapter.a
	$(CXX) $(CPPFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

libjson-adapter.a: $(ALL_OBJECTS)
	$(AR) rcs $@ $^

gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -o $@ -c $<

gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -o $@ -c $<

%.o: %.cc %.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $<

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.d: %.cc
	@set -e; rm -f $@; \
	$(CXX) -MM $(CPPFLAGS) $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# If only the goal 'clean' is given, do not generate and include the '%.d' files.
ifneq ($(MAKECMDGOALS),clean)
    -include $(DEPENDS)
endif

.PHONY: install
install: all
	mkdir -v -p "$(PREFIX)/bin" "$(PREFIX)/lib"
	mkdir -v -p "$(HTML_DIRECTORY)"
	cp -v ../html/* "$(HTML_DIRECTORY)"
	cp -v libjson-adapter.a "$(PREFIX)/lib"
	cp -v "$(TARGET)" "$(TARGET_TEST)" "$(TARGET_GTEST)" "$(PREFIX)/bin"

.PHONY: clean
clean:
	rm -vf "$(TARGET)" "$(TARGET_TEST)" "$(TARGET_GTEST)"
	rm -vf *.o json_spirit/*.o
	rm -vf *.a json_spirit/*.a
	rm -vf *.d json_spirit/*.d

.PHONY: run
run:
	@echo "The adapter must be installed for $< to work"
	$(CMD_PFX) "$(PREFIX)/bin/$(TARGET)"

.PHONY: test
test:
	@echo "The adapter must be installed for $< to work"
	$(CMD_PFX) "$(PREFIX)/bin/$(TARGET_TEST)"
	$(CMD_PFX) "$(PREFIX)/bin/$(TARGET_GTEST)"
