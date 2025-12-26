# Makefile for EPON Manager Telemetry Library (Dummy/Stub Version)

CC = gcc
CFLAGS = -Wall -Wextra -fPIC -O2 -I../../include -I../logger
LDFLAGS = -shared -L../logger -leponMgr_logger -lpthread

# Target library
TARGET = libepon_telemetry.so

# Source files
SRCS = eponMgr_telemetry.c

# Object files
OBJS = $(SRCS:.c=.o)

# Build directory for shared library
LIB_DIR = ../../lib

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Building telemetry library (dummy/stub version)..."
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)
	@mkdir -p $(LIB_DIR)
	@cp $(TARGET) $(LIB_DIR)/
	@echo "Built and installed: $(LIB_DIR)/$(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(LIB_DIR)/$(TARGET)
	@echo "Cleaned telemetry library"

rebuild: clean all

.PHONY: all clean rebuild
