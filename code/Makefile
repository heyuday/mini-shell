# Compiler and Flags
CC = gcc
CFLAGS_COMMON = -std=gnu18 -Wall -Wextra -Werror -pedantic
CFLAGS_RELEASE = $(CFLAGS_COMMON) -O2
CFLAGS_DEBUG = $(CFLAGS_COMMON) -Og -ggdb

# Target executables
TARGET = wsh
TARGET_DEBUG = $(TARGET)-dbg

# Source and header files
SRC = wsh.c dynamic_array.c utils.c hash_map.c
HDR = wsh.h dynamic_array.h utils.h hash_map.h

# Build directories
BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
DEBUG_DIR = $(BUILD_DIR)/debug

# Object files
OBJ_RELEASE = $(patsubst %.c,$(RELEASE_DIR)/%.o,$(SRC))
OBJ_DEBUG = $(patsubst %.c,$(DEBUG_DIR)/%.o,$(SRC))

# Default target
all: $(TARGET) $(TARGET_DEBUG)

# Release build
$(TARGET): $(OBJ_RELEASE)
	$(CC) $(CFLAGS_RELEASE) $^ -o $@

# Debug build
$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CFLAGS_DEBUG) $^ -o $@

# Compile release objects
$(RELEASE_DIR)/%.o: %.c $(HDR) | $(RELEASE_DIR)
	$(CC) $(CFLAGS_RELEASE) -c $< -o $@

# Compile debug objects
$(DEBUG_DIR)/%.o: %.c $(HDR) | $(DEBUG_DIR)
	$(CC) $(CFLAGS_DEBUG) -c $< -o $@

# Ensure directories exist
$(RELEASE_DIR) $(DEBUG_DIR):
	mkdir -p $@

# Cleanup
clean:
	rm -rf $(BUILD_DIR) $(TARGET) $(TARGET_DEBUG)

.PHONY: all clean
