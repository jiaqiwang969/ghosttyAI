# Makefile for Layout Management
# Task: T-203 - Layout Management Callbacks
# Author: CORE-001 (c-tmux-specialist)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread -I../include -I../../CORE-002/include -I../../src
LDFLAGS = -lpthread -lm

# Source files
SRCS = src/layout_manager.c src/pane_operations.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = tests/test_layout.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Targets
LIB = liblayout.a
TEST_BIN = test_layout

.PHONY: all clean test

all: $(LIB) $(TEST_BIN)

$(LIB): $(OBJS)
	ar rcs $@ $^

$(TEST_BIN): $(TEST_OBJS) $(LIB)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(LIB) $(TEST_BIN)

# Pattern rules
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies
src/layout_manager.o: src/layout_manager.c include/layout_callbacks.h
src/pane_operations.o: src/pane_operations.c include/layout_callbacks.h
tests/test_layout.o: tests/test_layout.c include/layout_callbacks.h