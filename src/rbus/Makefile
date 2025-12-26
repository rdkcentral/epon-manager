# Makefile for EPON Manager RBUS Integration

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I../../include -I../../src/logger -I../../src/core/hal_wrapper -I../../src/core/data_structures -I../../src/core/config -DUSE_DUMMY_RBUS
AR = ar
ARFLAGS = rcs

# Output
TARGET = libeponMgr_rbus.a

# Source files
SRCS = eponMgr_rbus.c \
       wanmanager/eponMgr_wanmanager.c \
       tr181/eponMgr_tr181.c \
       dummy/rbus_dummy.c

# Object files
OBJS = $(SRCS:.c=.o)

# Dependencies
DEPS = ../../src/logger/libeponMgr_logger.a

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $(TARGET) $(OBJS)
	@echo "Built: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned RBUS build"

.PHONY: all clean

