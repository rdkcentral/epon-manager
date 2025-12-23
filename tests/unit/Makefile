# Makefile for EPON Manager Unit Tests

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../src/logger
LDFLAGS = -L../../src/logger -lepon_logger

# Targets
TEST_LOGGER = test_logger

all: $(TEST_LOGGER)

$(TEST_LOGGER): test_logger.c ../../src/logger/libepon_logger.a
	$(CC) $(CFLAGS) test_logger.c -o $(TEST_LOGGER) $(LDFLAGS)
	@echo "Built: $(TEST_LOGGER)"

test: $(TEST_LOGGER)
	./$(TEST_LOGGER)

clean:
	rm -f $(TEST_LOGGER)
	@echo "Cleaned unit tests"

.PHONY: all test clean
