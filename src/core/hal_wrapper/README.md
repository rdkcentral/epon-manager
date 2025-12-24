# EPON Manager HAL Wrapper

## Overview

The HAL wrapper provides a caching layer for all EPON HAL APIs. It implements two caching strategies:

1. **TTL-based caching** (30 seconds default) for statistics APIs
2. **Validity flag caching** (no TTL) for info APIs - invalidated on ONU status changes

## Architecture

```
eponMgr_hal_wrapper_t
├── eponMgr_cache_t           (Statistics cache with TTL)
├── eponMgr_interface_list_t  (Interface management)
├── eponMgr_llid_list_t       (LLID management)
├── eponMgr_cpe_list_t        (CPE MAC table)
├── eponMgr_onu_state_t       (ONU status & info cache)
└── pthread_mutex_t           (Thread safety)
```

## Caching Strategy

### Statistics APIs (TTL-based, 30s default)
- `get_link_stats()` - Check cache → On miss, call HAL → Update cache
- `get_transceiver_stats()` - Check cache → On miss, call HAL → Update cache

### Info APIs (Validity flag, no TTL)
- `get_olt_info()` - Check validity → On miss, call HAL → Update state
- `get_onu_manufacturer_info()` - Check validity → On miss, call HAL → Update state
- `get_link_info()` - Check validity → On miss, call HAL → Update state

**Note:** For ONU status, use `eponMgr_onu_state_get_status()` directly from the ONU state data structure.

### List APIs (Always call HAL, update data structures)
- `get_llid_info()` - Call HAL → Clear → Populate LLID list
- `get_interface_list()` - Call HAL → Clear → Populate interface list
- `get_cpe_mac_table()` - Call HAL → Clear → Populate CPE list

### Configuration APIs (Invalidate cache on change)
- `set_oam_log_mask()` - Call HAL (no caching)

**Note:** MAC management and encryption configuration APIs are not provided by the HAL.

## Implemented APIs

### Initialization
- `eponMgr_hal_wrapper_init()` - Initialize wrapper and all data structures
- `eponMgr_hal_wrapper_destroy()` - Cleanup all resources
- `eponMgr_hal_wrapper_hal_init()` - Initialize HAL

### Statistics (with TTL cache)
- `eponMgr_hal_wrapper_get_link_stats()`
- `eponMgr_hal_wrapper_get_transceiver_stats()`

### Information (with validity flags)
- `eponMgr_hal_wrapper_get_olt_info()`
- `eponMgr_hal_wrapper_get_onu_manufacturer_info()`
- `eponMgr_hal_wrapper_get_link_info()`

### Lists (updates data structures)
- `eponMgr_hal_wrapper_get_llid_info()`
- `eponMgr_hal_wrapper_get_interface_list()`
- `eponMgr_hal_wrapper_get_cpe_mac_table()`
- `eponMgr_hal_wrapper_get_max_cpe()`

### Configuration
- `eponMgr_hal_wrapper_set_oam_log_level()` - Calls `epon_hal_set_oam_log_mask()`

### Cache Management
- `eponMgr_hal_wrapper_invalidate_cache()` - Invalidate all cache entries

### Direct Data Structure Access
To access internal data structures directly, use the public fields of `eponMgr_hal_wrapper_t`:
- `wrapper->interface_list` - Access interface list
- `wrapper->llid_list` - Access LLID list
- `wrapper->cpe_list` - Access CPE list
- `wrapper->onu_state` - Access ONU state

## Usage Example

```c
#include "eponMgr_hal_wrapper.h"

// Callback handlers
void status_callback(epon_onu_status_t status, void *user_data) {
    eponMgr_hal_wrapper_t *wrapper = (eponMgr_hal_wrapper_t *)user_data;
    // Update ONU state directly
    eponMgr_onu_state_update_status(wrapper->onu_state, status);
    // Invalidate cache on status change
    eponMgr_hal_wrapper_invalidate_cache(wrapper);
}

int main() {
    eponMgr_hal_wrapper_t wrapper;
    epon_hal_config_t config;
    
    // Setup config with callbacks
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.status_callback = status_callback;
    config.user_data = &wrapper;
    
    // Initialize wrapper (30s cache TTL)
    if (eponMgr_hal_wrapper_init(&wrapper, &config, 30) != 0) {
        fprintf(stderr, "Failed to initialize HAL wrapper\n");
        return -1;
    }
    
    // Initialize HAL
    if (eponMgr_hal_wrapper_hal_init(&wrapper) != EPON_HAL_SUCCESS) {
        fprintf(stderr, "Failed to initialize HAL\n");
        eponMgr_hal_wrapper_destroy(&wrapper);
        return -1;
    }
    
    // Get link stats (will cache for 30s)
    epon_hal_link_stats_t stats;
    stats.struct_size = sizeof(stats);
    if (eponMgr_hal_wrapper_get_link_stats(&wrapper, &stats) == EPON_HAL_SUCCESS) {
        printf("Bytes TX: %lu\n", stats.bytes_tx);
        // Second call within 30s will be served from cache
    }
    
    // Get OLT info (cached with validity flag)
    epon_olt_info_t olt_info;
    olt_info.struct_size = sizeof(olt_info);
    if (eponMgr_hal_wrapper_get_olt_info(&wrapper, &olt_info) == EPON_HAL_SUCCESS) {
        printf("OLT: %s\n", olt_info.olt_vendor_id);
        // Will be cached until ONU status changes
    }
    
    // Get ONU status directly from state
    epon_onu_status_t status;
    if (eponMgr_onu_state_get_status(wrapper.onu_state, &status) == 0) {
        printf("ONU Status: %d\n", status);
    }
    
    // Cleanup
    eponMgr_hal_wrapper_destroy(&wrapper);
    return 0;
}
```

## Thread Safety

All wrapper functions are thread-safe. The wrapper uses a single `pthread_mutex_t` to protect:
- Cache access (both read and write)
- Data structure updates
- HAL API calls

## Dependencies

- `libeponMgr_datastructures.a` - Core data structures (cache, lists, state)
- `epon_hal.h` - EPON HAL interface

## Build

```bash
cd src/core/hal_wrapper
make
# Output: libeponMgr_hal_wrapper.a
```

## Files

- `eponMgr_hal_wrapper.h` - API declarations
- `eponMgr_hal_wrapper.c` - Implementation with caching logic
- `Makefile` - Build configuration
- `README.md` - This file

## Notes

1. **Simplified API**: The wrapper only exposes functions for HAL operations. Internal data structures (interface_list, llid_list, cpe_list, onu_state) can be accessed directly through the wrapper struct fields.

2. **ONU Status**: Use `eponMgr_onu_state_get_status()` directly on `wrapper->onu_state` to get the current status. Status is updated via callbacks.

3. **Cache Invalidation**: The cache should be invalidated when ONU status changes. This is typically done in the status callback handler.

4. **Statistics TTL**: The 30-second TTL for statistics prevents excessive HAL calls while ensuring reasonably fresh data.

5. **Info Validity**: Info APIs use validity flags instead of TTL because this data rarely changes but must be invalidated on ONU status transitions.

6. **List Updates**: List APIs always call the HAL but update internal data structures for efficient iteration and lookups.
