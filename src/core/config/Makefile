# Makefile for EPON Config

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -fPIC
LDFLAGS = 

# Output
TARGET = libeponMgr_config.a
OBJS = eponMgr_config.o

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $(OBJS)
	@echo "Built: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned config build"

.PHONY: all clean
