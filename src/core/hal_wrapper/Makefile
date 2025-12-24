# Makefile for EPON Manager HAL Wrapper

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I../../../include -I../config -I../data_structures -fPIC
AR = ar
ARFLAGS = rcs

# Source files
SRCS = eponMgr_hal_wrapper.c
OBJS = $(SRCS:.c=.o)

# Target library
TARGET = libeponMgr_hal_wrapper.a

# Dependencies
DEPS = ../data_structures/libeponMgr_datastructures.a

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Building HAL wrapper library..."
	$(AR) $(ARFLAGS) $@ $(OBJS)
	@echo "Built: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned HAL wrapper build"

.PHONY: all clean
