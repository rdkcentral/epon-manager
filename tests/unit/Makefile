# Makefile for EPON Manager Unit Tests

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../src/logger -I../../src/core/config -I../../src/core/data_structures -I../../include
LDFLAGS_LOGGER = -L../../src/logger -leponMgr_logger
LDFLAGS_CONFIG = -L../../src/core/config -leponMgr_config -lpthread
LDFLAGS_DATA = -L../../src/core/data_structures -leponMgr_datastructures -lpthread
LDFLAGS_RBUS = -L../../src/rbus -leponMgr_rbus \
               -L../../src/core/hal_wrapper -leponMgr_hal_wrapper \
               -L../../src/core/data_structures -leponMgr_datastructures \
               -L../../src/core/config -leponMgr_config \
               -L../../src/logger -leponMgr_logger \
               -L../../lib -lepon_hal_mock \
               -lpthread -Wl,-rpath,../../lib
LDFLAGS_TELEMETRY = -L../../lib -lepon_telemetry -L../../src/logger -leponMgr_logger -lpthread -Wl,-rpath,../../lib

# Targets
TEST_LOGGER = test_logger
TEST_CONFIG = test_config
TEST_CACHE = test_cache
TEST_QUEUE = test_queue
TEST_DATASTRUCTURES = test_datastructures
TEST_RBUS = test_rbus_basic
TEST_TELEMETRY = test_telemetry

TESTS = $(TEST_LOGGER) $(TEST_CONFIG) $(TEST_CACHE) $(TEST_QUEUE) $(TEST_DATASTRUCTURES) $(TEST_RBUS) $(TEST_TELEMETRY)

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

$(TEST_RBUS): test_rbus_basic.c ../../src/rbus/libeponMgr_rbus.a ../../src/logger/libeponMgr_logger.a
	$(CC) $(CFLAGS) test_rbus_basic.c -o $(TEST_RBUS) $(LDFLAGS_RBUS)
	@echo "Built: $(TEST_RBUS)"

$(TEST_TELEMETRY): test_telemetry.c ../../lib/libepon_telemetry.so ../../src/logger/libeponMgr_logger.a
	$(CC) $(CFLAGS) test_telemetry.c -o $(TEST_TELEMETRY) $(LDFLAGS_TELEMETRY)
	@echo "Built: $(TEST_TELEMETRY)"

test: $(TESTS)
	@echo "=== Running Unit Tests ==="
	@echo ""
	./$(TEST_LOGGER) && echo "" || exit 1
	./$(TEST_RBUS) && echo "" || exit 1
	./$(TEST_CONFIG) && echo "" || exit 1
	./$(TEST_CACHE) && echo "" || exit 1
	./$(TEST_QUEUE) && echo "" || exit 1
	./$(TEST_DATASTRUCTURES) && echo "" || exit 1
	./$(TEST_TELEMETRY) && echo "" || exit 1
	@echo "=== All Unit Tests Passed ==="

clean:
	rm -f $(TESTS)
	@echo "Cleaned unit tests"

.PHONY: all test clean
