CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2

WATCHDOG_SRCS = $(wildcard src/*.c)
HEADERS = $(wildcard include/*.h)

.PHONY: all clean test test-logger

all: watchdog workers/worker

watchdog: $(WATCHDOG_SRCS) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WATCHDOG_SRCS) -o $@

workers/worker: workers/worker.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

test: all
	bash tests/test_scenarios.sh

test-logger:
	$(CC) $(CPPFLAGS) $(CFLAGS) src/logger.c tests/logger_smoke.c -o tests/logger_smoke
	./tests/logger_smoke

clean:
	rm -f watchdog workers/worker tests/logger_smoke
