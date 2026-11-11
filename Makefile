# SPDX-License-Identifier: MIT
CXX ?= g++
CC ?= gcc
AR ?= ar

CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic
CPPFLAGS ?= -Iinclude -Ithird_party/jsmn

BUILD ?= build
LIB := $(BUILD)/libjsonlib.a
TEST := $(BUILD)/test_jsonlib
EXAMPLES := $(patsubst examples/%.cpp,$(BUILD)/examples/%,$(filter-out examples/writer_span.cpp,$(wildcard examples/*.cpp)))
EXAMPLES_CXX20 := $(BUILD)/examples/writer_span

CXX_OBJ := $(BUILD)/src/json_reader.o $(BUILD)/src/json_writer.o
C_OBJ := $(BUILD)/src/jsmn.o
OBJ := $(CXX_OBJ) $(C_OBJ)

.PHONY: all test examples examples-cxx20 check-examples check-examples-cxx20 clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(BUILD)/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/src/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST): tests/test_jsonlib.cpp $(LIB)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/%: examples/%.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/writer_span: examples/writer_span.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -std=c++20 -O2 -Wall -Wextra -Wpedantic $< $(LIB) -o $@

examples: $(EXAMPLES)

examples-cxx20: $(EXAMPLES_CXX20)

check-examples: $(EXAMPLES)
	@set -e; for ex in $(EXAMPLES); do echo "== $$ex"; ./$$ex; done

check-examples-cxx20: $(EXAMPLES_CXX20)
	@./$(EXAMPLES_CXX20)

test: $(TEST) check-examples
	./$(TEST)

clean:
	rm -rf $(BUILD)
