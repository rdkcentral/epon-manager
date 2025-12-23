# Makefile for EPON Manager Unit Tests

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../src/logger -I../../src/core/config -I../../src/core/data_structures -I../../include
LDFLAGS_LOGGER = -L../../src/logger -leponMgr_logger
LDFLAGS_CONFIG = -L../../src/core/config -leponMgr_config -lpthread
LDFLAGS_DATA = -L../../src/core/data_structures -leponMgr_datastructures -lpthread

# Targets
TEST_LOGGER = test_logger
TEST_CONFIG = test_config
TEST_CACHE = test_cache
TEST_QUEUE = test_queue
TEST_DATASTRUCTURES = test_datastructures

TESTS = $(TEST_LOGGER) $(TEST_CONFIG) $(TEST_CACHE) $(TEST_QUEUE) $(TEST_DATASTRUCTURES)

all: $(TESTS)

$(TEST_LOGGER): test_logger.c ../../src/logger/libeponMgr_logger.a
	$(CC) $(CFLAGS) test_logger.c -o $(TEST_LOGGER) $(LDFLAGS_LOGGER)
	@echo "Built: $(TEST_LOGGER)"

$(TEST_CONFIG): test_config.c ../../src/core/config/libeponMgr_config.a
	$(CC) $(CFLAGS) test_config.c -o $(TEST_CONFIG) $(LDFLAGS_CONFIG)
	@echo "Built: $(TEST_CONFIG)"

$(TEST_CACHE): test_cache.c ../../src/core/data_structures/libeponMgr_datastructures.a
	$(CC) $(CFLAGS) test_cache.c -o $(TEST_CACHE) $(LDFLAGS_DATA)
	@echo "Built: $(TEST_CACHE)"

$(TEST_QUEUE): test_queue.c ../../src/core/data_structures/libeponMgr_datastructures.a
	$(CC) $(CFLAGS) test_queue.c -o $(TEST_QUEUE) $(LDFLAGS_DATA)
	@echo "Built: $(TEST_QUEUE)"

$(TEST_DATASTRUCTURES): test_datastructures.c ../../src/core/data_structures/libeponMgr_datastructures.a
	$(CC) $(CFLAGS) test_datastructures.c -o $(TEST_DATASTRUCTURES) $(LDFLAGS_DATA)
	@echo "Built: $(TEST_DATASTRUCTURES)"

$(TEST_DATASTRUCTURES): test_datastructures.c ../../src/core/data_structures/libeponMgr_datastructures.a
	$(CC) $(CFLAGS) test_datastructures.c -o $(TEST_DATASTRUCTURES) $(LDFLAGS_DATA)
	@echo "Built: $(TEST_DATASTRUCTURES)"

test: $(TESTS)
	@echo "=== Running Unit Tests ==="
	@echo ""
	./$(TEST_LOGGER) && echo "" || exit 1
	./$(TEST_CONFIG) && echo "" || exit 1
	./$(TEST_CACHE) && echo "" || exit 1
	./$(TEST_QUEUE) && echo "" || exit 1
	./$(TEST_DATASTRUCTURES) && echo "" || exit 1
	@echo "=== All Unit Tests Passed ==="

clean:
	rm -f $(TESTS)
	@echo "Cleaned unit tests"

.PHONY: all test clean
