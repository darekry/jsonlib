# SPDX-License-Identifier: MIT
CXX ?= g++
CC ?= gcc
AR ?= ar

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude -Ithird_party/jsmn

# Reader backend: 0 = jsmn tokens, 1 = span scanner, 2 = hybrid.
JSONLIB_READER_MODE ?= 2
ifneq ($(filter 0 1 2,$(JSONLIB_READER_MODE)),$(JSONLIB_READER_MODE))
$(error JSONLIB_READER_MODE must be 0, 1 or 2, got '$(JSONLIB_READER_MODE)')
endif
READER_DEFS := -DJSONLIB_READER_MODE=$(JSONLIB_READER_MODE) -DJSMN_STRICT
ALL_CPPFLAGS = $(CPPFLAGS) $(READER_DEFS)

BUILD ?= build
LIB := $(BUILD)/libjsonlib.a
TEST := $(BUILD)/test_jsonlib
EXAMPLES := $(patsubst examples/%.cpp,$(BUILD)/examples/%,$(filter-out examples/writer_span.cpp,$(wildcard examples/*.cpp)))
EXAMPLES_CXX20 := $(BUILD)/examples/writer_span

# All sources are built in every mode so each TU is checked against each
# configuration; json_doc.cpp compiles out the unused backend.
CXX_OBJ := $(BUILD)/src/json_doc.o $(BUILD)/src/json_primitives.o \
	$(BUILD)/src/json_reader.o $(BUILD)/src/json_span_reader.o \
	$(BUILD)/src/json_writer.o
C_OBJ := $(BUILD)/src/jsmn.o
OBJ := $(CXX_OBJ) $(C_OBJ)

.PHONY: all test test-only test-modes examples examples-cxx20 \
	check-examples check-examples-cxx20 clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(BUILD)/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/src/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(ALL_CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST): tests/test_jsonlib.cpp $(LIB)
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/%: examples/%.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/writer_span: examples/writer_span.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(ALL_CPPFLAGS) -std=c++20 -O2 -Wall -Wextra -Wpedantic $< $(LIB) -o $@

examples: $(EXAMPLES)

examples-cxx20: $(EXAMPLES_CXX20)

check-examples: $(EXAMPLES)
	@set -e; for ex in $(EXAMPLES); do echo "== $$ex"; ./$$ex; done

check-examples-cxx20: $(EXAMPLES_CXX20)
	@./$(EXAMPLES_CXX20)

test-only: $(TEST)
	./$(TEST)

test: test-only check-examples

test-modes:
	$(MAKE) JSONLIB_READER_MODE=0 BUILD=build/mode0 test-only
	$(MAKE) JSONLIB_READER_MODE=1 BUILD=build/mode1 test-only
	$(MAKE) JSONLIB_READER_MODE=2 BUILD=build/mode2 test-only

clean:
	rm -rf $(BUILD)
