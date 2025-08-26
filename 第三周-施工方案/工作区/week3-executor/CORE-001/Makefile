# Makefile for Event Loop Backend
# Task: T-201 - Event Loop vtable abstraction
# Author: CORE-001 (c-tmux-specialist)

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g -pthread -I../src
LDFLAGS = -levent -lpthread

# Source files
SRCS = src/event_loop_router.c
OBJS = $(SRCS:.c=.o)

# Test files
TEST_SRCS = tests/test_event_loop.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Targets
LIB = libeventloop.a
TEST_BIN = test_event_loop

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
src/event_loop_router.o: src/event_loop_router.c src/event_loop_backend.h
tests/test_event_loop.o: tests/test_event_loop.c src/event_loop_backend.h