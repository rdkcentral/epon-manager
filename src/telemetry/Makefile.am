# Makefile.am for telemetry library

if ENABLE_TELEMETRY

lib_LTLIBRARIES = libepon_telemetry.la

libepon_telemetry_la_SOURCES = \
    eponMgr_telemetry.c

libepon_telemetry_la_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    $(AM_CFLAGS)

libepon_telemetry_la_LIBADD = \
    $(top_builddir)/src/logger/libeponMgr_logger.la

libepon_telemetry_la_LDFLAGS = \
    -version-info 1:0:0

endif
