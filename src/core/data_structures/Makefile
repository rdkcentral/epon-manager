# Makefile for EPON Data Structures

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I. -I../../../include -fPIC
LDFLAGS = -lpthread

# Output
TARGET = libeponMgr_datastructures.a
OBJS = eponMgr_cache.o eponMgr_queue.o eponMgr_interface_list.o eponMgr_llid_list.o eponMgr_cpe_list.o eponMgr_onu_state.o

# Build targets
all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $(TARGET) $(OBJS)
	@echo "Built: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned data structures build"

.PHONY: all clean
