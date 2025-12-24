# Makefile for EPON Manager Main Application

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I../../include -I../logger
LDFLAGS = -L./controller -L./hal_wrapper -L./data_structures -L./config -L../logger -L../../lib \
          -leponMgr_controller -leponMgr_hal_wrapper -leponMgr_datastructures \
          -leponMgr_config -leponMgr_logger -lepon_hal_mock -lpthread -Wl,-rpath,../../lib

# Main executable
TARGET = epon_manager

# Source files
SRCS = epon_manager_main.c

# Object files
OBJS = $(SRCS:.c=.o)

# Libraries to build first
LIBS = ../logger/libeponMgr_logger.a \
       config/libeponMgr_config.a \
       data_structures/libeponMgr_datastructures.a \
       hal_wrapper/libeponMgr_hal_wrapper.a \
       controller/libeponMgr_controller.a \
       ../../lib/libepon_hal_mock.so

all: $(LIBS) $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking EPON Manager application..."
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Built: $(TARGET)"

../logger/libeponMgr_logger.a:
	@echo "Building logger library..."
	$(MAKE) -C ../logger

config/libeponMgr_config.a:
	@echo "Building config library..."
	$(MAKE) -C config

data_structures/libeponMgr_datastructures.a:
	@echo "Building data structures library..."
	$(MAKE) -C data_structures

hal_wrapper/libeponMgr_hal_wrapper.a:
	@echo "Building HAL wrapper library..."
	$(MAKE) -C hal_wrapper

controller/libeponMgr_controller.a:
	@echo "Building controller library..."
	$(MAKE) -C controller

../../lib/libepon_hal_mock.so:
	@echo "Building HAL mock library..."
	$(MAKE) -C ../../tests/hal_mock

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	$(MAKE) -C ../logger clean
	$(MAKE) -C config clean
	$(MAKE) -C data_structures clean
	$(MAKE) -C hal_wrapper clean
	$(MAKE) -C controller clean
	$(MAKE) -C ../../tests/hal_mock clean
	@echo "Cleaned EPON Manager build"

.PHONY: all clean
