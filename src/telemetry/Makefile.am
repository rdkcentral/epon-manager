# Makefile.am for telemetry library

if ENABLE_TELEMETRY

noinst_LTLIBRARIES = libepon_telemetry.la

libepon_telemetry_la_SOURCES = \
    eponMgr_telemetry.c

libepon_telemetry_la_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    $(AM_CFLAGS)


endif
