# Makefile for EPON HAL Mock

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I../../include -fPIC
LDFLAGS = -shared

# Output
TARGET = ../../lib/libepon_hal_mock.so
OBJS = epon_hal_mock.o

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p ../../lib
	$(CC) $(LDFLAGS) $(OBJS) -o $(TARGET)
	@echo "Built: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned HAL mock build"

.PHONY: all clean
