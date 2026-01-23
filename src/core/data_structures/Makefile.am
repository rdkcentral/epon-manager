# Makefile.am for data_structures library

noinst_LIBRARIES = libeponMgr_datastructures.a

libeponMgr_datastructures_a_SOURCES = \
    eponMgr_data.c \
    eponMgr_data.h \
    eponMgr_statsData.c \
    eponMgr_statsData.h \
    eponMgr_queue.c \
    eponMgr_queue.h \
    eponMgr_onu_state.c \
    eponMgr_onu_state.h

libeponMgr_datastructures_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/data_structures \
    $(AM_CFLAGS)
