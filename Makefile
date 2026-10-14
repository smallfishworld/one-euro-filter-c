CC ?= cc
AR ?= ar
CFLAGS ?= -O2 -Wall -Wextra -Wpedantic -std=c99
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?= -lm

BUILD_DIR := build-make
LIB := $(BUILD_DIR)/libone_euro_filter.a
OBJ := $(BUILD_DIR)/one_euro_filter.o $(BUILD_DIR)/one_euro_auto.o $(BUILD_DIR)/one_euro_clock.o
EXAMPLES := $(BUILD_DIR)/oef_example_1d \
            $(BUILD_DIR)/oef_example_2d \
            $(BUILD_DIR)/oef_example_3d

.PHONY: all test examples example clean
all: $(LIB)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/one_euro_filter.o: src/one_euro_filter.c include/one_euro_filter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/one_euro_auto.o: src/one_euro_auto.c include/one_euro_filter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -c $< -o $@

$(BUILD_DIR)/one_euro_clock.o: src/one_euro_clock.c include/one_euro_filter.h | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(BUILD_DIR)/oef_tests: tests/test_one_euro_filter.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/oef_error_tests: tests/test_error_codes.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

test: $(BUILD_DIR)/oef_tests $(BUILD_DIR)/oef_error_tests $(BUILD_DIR)/oef_auto_tests
	./$(BUILD_DIR)/oef_tests
	./$(BUILD_DIR)/oef_error_tests
	./$(BUILD_DIR)/oef_auto_tests

$(BUILD_DIR)/oef_auto_tests: tests/test_auto_clock.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/oef_example_1d: examples/one_dimensional.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/oef_example_2d: examples/two_dimensional.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

$(BUILD_DIR)/oef_example_3d: examples/three_dimensional.c $(LIB)
	$(CC) $(CPPFLAGS) $(CFLAGS) -Iinclude $< $(LIB) $(LDFLAGS) $(LDLIBS) -o $@

examples: $(EXAMPLES)

example: examples
	@for executable in $(EXAMPLES); do ./$$executable || exit 1; done

clean:
	rm -rf $(BUILD_DIR)
