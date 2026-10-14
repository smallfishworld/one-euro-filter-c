CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c99
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -lm

BUILD_DIR := build-make
LIB := $(BUILD_DIR)/libone_euro_filter.a
OBJ := $(BUILD_DIR)/one_euro_filter.o

.PHONY: all test example clean
all: $(LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ): src/one_euro_filter.c include/one_euro_filter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(BUILD_DIR)/oef_tests: tests/test_one_euro_filter.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

test: $(BUILD_DIR)/oef_tests
	./$(BUILD_DIR)/oef_tests

$(BUILD_DIR)/oef_acoustic_example: examples/acoustic_imaging.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

example: $(BUILD_DIR)/oef_acoustic_example
	./$(BUILD_DIR)/oef_acoustic_example

clean:
	rm -rf $(BUILD_DIR)
