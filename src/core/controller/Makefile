# Makefile for EPON Manager Controller

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I../../../include -I../../logger -I../config -I../hal_wrapper -I../data_structures -fPIC
AR = ar
ARFLAGS = rcs

# Source files
SRCS = eponMgr_controller.c
OBJS = $(SRCS:.c=.o)

# Target library
TARGET = libeponMgr_controller.a

# Dependencies
DEPS = ../hal_wrapper/libeponMgr_hal_wrapper.a \
       ../data_structures/libeponMgr_datastructures.a \
       ../config/libeponMgr_config.a \
       ../../logger/libepon_logger.a

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Building controller library..."
	$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned controller build"

.PHONY: all clean
