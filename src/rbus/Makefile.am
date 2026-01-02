# Makefile.am for RBUS library

if ENABLE_RBUS

noinst_LIBRARIES = libeponMgr_rbus.a

libeponMgr_rbus_a_SOURCES = \
    eponMgr_rbus.c \
    eponMgr_psm.c \
    eponMgr_psm.h \
    tr181/eponMgr_tr181.c \
    tr181/eponMgr_tr181.h \
    wanmanager/eponMgr_wanmanager.c

libeponMgr_rbus_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/data_structures \
    -I$(top_srcdir)/src/core/stats_poller \
    -I$(top_srcdir)/src/core/controller \
    $(AM_CFLAGS)

endif
