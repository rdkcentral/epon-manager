# Makefile for EPON Logger

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -fPIC
LDFLAGS = 

# Output
TARGET = libepon_logger.a
OBJS = epon_logger.o

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $(OBJS)
	@echo "Built: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned logger build"

.PHONY: all clean
