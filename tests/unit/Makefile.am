# Makefile.am for unit tests

if ENABLE_TESTS

# Test programs
check_PROGRAMS = \
    test_logger \
    test_config \
    test_cache \
    test_queue \
    test_datastructures \
    test_rbus_basic \
    test_telemetry

# Common flags for all tests
AM_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/data_structures

# Test: Logger
test_logger_SOURCES = test_logger.c
test_logger_LDADD = \
    $(top_builddir)/src/logger/libeponMgr_logger.la

# Test: Config
test_config_SOURCES = test_config.c
test_config_LDADD = \
    $(top_builddir)/src/core/config/libeponMgr_config.a \
    -lpthread

# Test: Cache
test_cache_SOURCES = test_cache.c
test_cache_LDADD = \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    -lpthread

# Test: Queue
test_queue_SOURCES = test_queue.c
test_queue_LDADD = \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    -lpthread

# Test: Data Structures
test_datastructures_SOURCES = test_datastructures.c
test_datastructures_LDADD = \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    -lpthread

# Test: RBUS Basic
test_rbus_basic_SOURCES = test_rbus_basic.c
test_rbus_basic_LDADD = \
    $(top_builddir)/src/rbus/libeponMgr_rbus.a \
    $(top_builddir)/src/core/hal_wrapper/libeponMgr_hal_wrapper.a \
    $(top_builddir)/src/core/data_structures/libeponMgr_datastructures.a \
    $(top_builddir)/src/core/config/libeponMgr_config.a \
    $(top_builddir)/src/logger/libeponMgr_logger.la \
    $(top_builddir)/tests/hal_mock/libepon_hal_mock.la \
    -lpthread

# Test: Telemetry
test_telemetry_SOURCES = test_telemetry.c
test_telemetry_LDADD = \
    $(top_builddir)/src/telemetry/libepon_telemetry.la \
    $(top_builddir)/src/logger/libeponMgr_logger.la \
    -lpthread

# Define tests to run
TESTS = $(check_PROGRAMS)

# Create logs directory for tests
check-local:
	$(MKDIR_P) logs

endif
