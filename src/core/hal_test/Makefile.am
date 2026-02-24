# Makefile.am for HAL test library

noinst_LIBRARIES = libeponMgr_hal_test.a

libeponMgr_hal_test_a_SOURCES = \
    eponMgr_hal_test.c \
    eponMgr_hal_test.h

libeponMgr_hal_test_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/data_structures \
    -I$(top_srcdir)/src/core/controller \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/stats_poller \
    -I$(top_srcdir)/src/rbus \
    -I$(top_srcdir)/src/rbus/tr181 \
    -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/epon \
    $(AM_CFLAGS)
