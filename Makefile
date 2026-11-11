# SPDX-License-Identifier: MIT
CXX ?= g++
CC ?= gcc
AR ?= ar

# Konfiguracja biblioteki NIE jest tutaj: wszystko jest w
# include/jsonlib/config.hpp (odkomentuj jedna linie na sekcje).
# Cele testowe moga nadpisac config przez -D na wiersz polecen
# (test-modes, test-nofloat) - normalny build uzywa pliku.
# -Os: biblioteka celuje w STM32/flash, rozmiar wazniejszy niz ostatnie 5% CPU.
CXXFLAGS ?= -std=c++17 -Os -Wall -Wextra -Wpedantic -ffunction-sections -fdata-sections
CFLAGS ?= -Os -Wall -Wextra -Wpedantic -ffunction-sections -fdata-sections
CPPFLAGS ?= -Iinclude -Ithird_party/jsmn

# Nadpisanie trybu z linii komend (tylko test-modes). Puste = config.hpp.
JSONLIB_READER_MODE ?=
MODE_DEFS := $(if $(JSONLIB_READER_MODE),-DJSONLIB_READER_MODE=$(JSONLIB_READER_MODE))
NOFLOAT_DEFS := $(if $(JSONLIB_NO_FLOAT),-DJSONLIB_NO_FLOAT=$(JSONLIB_NO_FLOAT))
ALL_CPPFLAGS = $(CPPFLAGS) $(MODE_DEFS) $(NOFLOAT_DEFS)

BUILD ?= build
LIB := $(BUILD)/libjsonlib.a
TEST := $(BUILD)/test_jsonlib
EXAMPLES := $(patsubst examples/%.cpp,$(BUILD)/examples/%,$(filter-out examples/writer_span.cpp,$(wildcard examples/*.cpp)))
EXAMPLES_CXX20 := $(BUILD)/examples/writer_span

# Wszystkie zrodla sa budowane w kazdym trybie, zeby kazde TU bylo
# sprawdzane przeciwko kazdej konfiguracji; firmware z
# -ffunction-sections/-Wl,--gc-sections wycina nieuzywane sekcje.
# (jsmn.c dolacza config.hpp dla JSMN_STRICT.)
CXX_OBJ := $(BUILD)/src/json_doc.o $(BUILD)/src/json_primitives.o \
	$(BUILD)/src/json_reader.o $(BUILD)/src/json_span_reader.o \
	$(BUILD)/src/json_writer.o
C_OBJ := $(BUILD)/src/jsmn.o
OBJ := $(CXX_OBJ) $(C_OBJ)

.PHONY: all test test-only test-modes test-nofloat examples examples-cxx20 \
	check-examples check-examples-cxx20 clean

all: $(LIB)

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

# Zaleznosci naglowkowe: edycja config.hpp MUSI przebudowac biblioteke.
DEPFLAGS := -MMD -MP

$(BUILD)/src/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(DEPFLAGS) $(ALL_CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD)/src/%.o: src/%.c
	mkdir -p $(dir $@)
	$(CC) $(DEPFLAGS) $(ALL_CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(OBJ:.o=.d)

$(TEST): tests/test_jsonlib.cpp $(LIB)
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/%: examples/%.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(ALL_CPPFLAGS) $(CXXFLAGS) $< $(LIB) -o $@

$(BUILD)/examples/writer_span: examples/writer_span.cpp $(LIB)
	mkdir -p $(dir $@)
	$(CXX) $(ALL_CPPFLAGS) -std=c++20 -Os -Wall -Wextra -Wpedantic -ffunction-sections -fdata-sections $< $(LIB) -o $@

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

test-nofloat:
	$(MAKE) JSONLIB_NO_FLOAT=1 BUILD=build/nofloat test-only

clean:
	rm -rf $(BUILD)
