# Makefile.am for HAL mock library
# Provides mock EPON HAL implementation for RDK integration testing

if ENABLE_TESTS

lib_LTLIBRARIES = libepon_hal_mock.la

libepon_hal_mock_la_SOURCES = \
    epon_hal_mock.c

libepon_hal_mock_la_CFLAGS = \
    -I$(top_srcdir)/include \
    $(AM_CFLAGS)

libepon_hal_mock_la_LDFLAGS = \
    -version-info 1:0:0

endif
