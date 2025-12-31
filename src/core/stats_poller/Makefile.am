noinst_LIBRARIES = libeponMgr_stats_poller.a

libeponMgr_stats_poller_a_SOURCES = \
    eponMgr_stats_poller.c \
    eponMgr_stats_poller.h

libeponMgr_stats_poller_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/hal_wrapper \
    -I$(top_srcdir)/src/core/data_structures \
    $(RBUS_CFLAGS)

libeponMgr_stats_poller_a_LIBADD = \
    $(RBUS_LIBS)
