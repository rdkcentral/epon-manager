# Makefile.am for integration tests

if ENABLE_TESTS

# Integration test programs
check_PROGRAMS = test_event_processing

# Common flags
AM_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/tests/hal_mock

# Test: Event Processing
test_event_processing_SOURCES = test_event_processing.c
test_event_processing_LDADD = \
    $(top_builddir)/tests/hal_mock/libepon_hal_mock.la

# Define tests to run
TESTS = $(check_PROGRAMS)

endif
