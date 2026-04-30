# Makefile.am for telemetry library

if ENABLE_TELEMETRY

noinst_LIBRARIES = libepon_telemetry.a

libepon_telemetry_a_SOURCES = \
    eponMgr_telemetry.c \
    eponMgr_harvester.c \
    eponMgr_harvester_avro.c \
    eponMgr_harvester_webpa.c

libepon_telemetry_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/data_structures \
    -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/epon \
    $(AVRO_CFLAGS) \
    $(AM_CFLAGS)

# Install Avro schema where the harvester loader expects to find it.
harvesterdir = $(prefix)/ccsp/harvester
harvester_DATA = EponReport.avsc

EXTRA_DIST = EponReport.avsc

endif
