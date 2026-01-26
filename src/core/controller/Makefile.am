# Makefile.am for controller library

noinst_LIBRARIES = libeponMgr_controller.a

libeponMgr_controller_a_SOURCES = \
    eponMgr_controller.c \
    eponMgr_controller.h

libeponMgr_controller_a_CFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src/logger \
    -I$(top_srcdir)/src/core/config \
    -I$(top_srcdir)/src/core/data_structures \
    -I$(top_srcdir)/src/core/stats_poller \
    -I$(top_srcdir)/src/rbus \
    $(EPON_HAL_CFLAGS) \
    -DGIT_VERSION=\"$(GIT_VERSION)\" \
    $(AM_CFLAGS)
