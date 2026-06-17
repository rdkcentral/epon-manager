# Makefile.am for telemetry library

noinst_LIBRARIES = libepon_telemetry.a

libepon_telemetry_a_SOURCES = \
    eponMgr_telemetry.c

libepon_telemetry_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/data_structures \
    -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/epon \
    $(AM_CFLAGS)
