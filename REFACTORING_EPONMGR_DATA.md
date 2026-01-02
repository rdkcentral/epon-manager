# EPON Manager Data Structure Refactoring

## Summary

The `eponMgr_hal_wrapper` module has been renamed and reorganized to `eponMgr_data` to better reflect its purpose as the core data context for the EPON Manager.

**Note**: The old `src/core/hal_wrapper/` directory is now deprecated and has been removed from the build system. All functionality is now in `src/core/data_structures/`.

## Rationale

The original name `eponMgr_hal_wrapper_t` was misleading. This structure is not merely a "wrapper" around HAL APIs—it is the **central data container** for the entire EPON Manager, managing:

- Statistics cache with TTL
- State management (interfaces, LLIDs, CPEs, ONU state)
- HAL initialization and configuration
- Thread synchronization primitives

The new name `eponMgr_data_t` accurately reflects its role.

## Changes Made

### 1. New Files Created

**Location**: `src/core/data_structures/`

- `eponMgr_data.h` - Core data structure header
- `eponMgr_data.c` - Core data structure implementation

These replace the functionality previously in `src/core/hal_wrapper/`.

### 2. Type Renames

| Old Name | New Name |
|----------|----------|
| `eponMgr_hal_wrapper_t` | `eponMgr_data_t` |

### 3. Function Renames

All functions follow the pattern `eponMgr_hal_wrapper_*` → `eponMgr_data_*`:

| Old Function | New Function |
|--------------|--------------|
| `eponMgr_hal_wrapper_init()` | `eponMgr_data_init()` |
| `eponMgr_hal_wrapper_destroy()` | `eponMgr_data_destroy()` |
| `eponMgr_hal_wrapper_get_version()` | `eponMgr_data_get_hal_version()` |
| `eponMgr_hal_wrapper_hal_init()` | `eponMgr_data_hal_init()` |
| `eponMgr_hal_wrapper_get_link_stats()` | `eponMgr_data_get_link_stats()` |
| `eponMgr_hal_wrapper_get_transceiver_stats()` | `eponMgr_data_get_transceiver_stats()` |
| `eponMgr_hal_wrapper_get_llid_info()` | `eponMgr_data_get_llid_info()` |
| `eponMgr_hal_wrapper_get_interface_list()` | `eponMgr_data_get_interface_list()` |
| `eponMgr_hal_wrapper_get_olt_info()` | `eponMgr_data_get_olt_info()` |
| `eponMgr_hal_wrapper_get_onu_manufacturer_info()` | `eponMgr_data_get_onu_manufacturer_info()` |
| `eponMgr_hal_wrapper_get_link_info()` | `eponMgr_data_get_link_info()` |
| `eponMgr_hal_wrapper_get_max_cpe()` | `eponMgr_data_get_max_cpe()` |
| `eponMgr_hal_wrapper_get_cpe_mac_table()` | `eponMgr_data_get_cpe_mac_table()` |
| `eponMgr_hal_wrapper_set_oam_log_level()` | `eponMgr_data_set_oam_log_level()` |
| `eponMgr_hal_wrapper_invalidate_cache()` | `eponMgr_data_invalidate_cache()` |

### 4. New Locking API

Thread-safe access functions moved from controller to data module:

**New Functions in `eponMgr_data.h`**:
```c
int eponMgr_data_lock(eponMgr_data_t *eponData);
void eponMgr_data_unlock(eponMgr_data_t *eponData);
```

**Old Functions (removed)**:
```c
void* eponMgr_controller_lock_hal_wrapper(void);
void eponMgr_controller_unlock_hal_wrapper(void);
```

**Benefits**:
- Better encapsulation - locks use the data context's internal mutex
- Cleaner API - no global controller dependency
- More explicit - caller manages the data pointer

### 5. Controller API Changes

| Old Function | New Function |
|--------------|--------------|
| `eponMgr_controller_get_hal_wrapper()` | `eponMgr_controller_get_data()` |

**Controller struct member renamed**:
- `hal_wrapper` → `data`

### 6. Variable Naming Conventions

Throughout the codebase:
- `wrapper` → `eponData`
- `hal_wrapper` → `data` or `eponData`

### 7. Files Updated

All references updated in:
- `src/core/controller/eponMgr_controller.{c,h}`
- `src/core/stats_poller/eponMgr_stats_poller.{c,h}`
- `src/rbus/tr181/eponMgr_tr181.c`
- `include/eponMgr_tr181.h`
- `src/core/Makefile.am` (build system)
- `src/core/data_structures/Makefile.am` (build system)

### 8. Build System Changes

**Updated Makefiles**:
- Added `eponMgr_data.{c,h}` to `data_structures/Makefile.am`
- Removed `libeponMgr_hal_wrapper.a` from link dependencies
- Deprecated `hal_wrapper/` subdirectory (marked but not removed)

## Migration Examples

### Example 1: Basic Usage Pattern

**Before**:
```c
#include "hal_wrapper/eponMgr_hal_wrapper.h"

eponMgr_hal_wrapper_t *wrapper = get_wrapper();
epon_hal_link_stats_t stats = {0};
stats.struct_size = sizeof(stats);
eponMgr_hal_wrapper_get_link_stats(wrapper, &stats);
```

**After**:
```c
#include "data_structures/eponMgr_data.h"

eponMgr_data_t *eponData = get_data();
epon_hal_link_stats_t stats = {0};
stats.struct_size = sizeof(stats);
eponMgr_data_get_link_stats(eponData, &stats);
```

### Example 2: Thread-Safe Access (TR-181 Handlers)

**Before**:
```c
eponMgr_hal_wrapper_t *wrapper = eponMgr_controller_lock_hal_wrapper();
if (!wrapper) return RBUS_ERROR_BUS_ERROR;

// Use wrapper...
epon_hal_link_stats_t stats = {0};
eponMgr_hal_wrapper_get_link_stats(wrapper, &stats);

eponMgr_controller_unlock_hal_wrapper();
```

**After**:
```c
eponMgr_data_t *eponData = eponMgr_controller_get_data(eponMgr_controller_get_instance());
if (!eponData) return RBUS_ERROR_BUS_ERROR;

eponMgr_data_lock(eponData);

// Use eponData...
epon_hal_link_stats_t stats = {0};
eponMgr_data_get_link_stats(eponData, &stats);

eponMgr_data_unlock(eponData);
```

### Example 3: Controller Context

**Before**:
```c
struct eponMgr_controller_context {
    eponMgr_hal_wrapper_t *hal_wrapper;
    // ...
};

ctrl->hal_wrapper = malloc(sizeof(eponMgr_hal_wrapper_t));
eponMgr_hal_wrapper_init(ctrl->hal_wrapper, &config, ttl);
```

**After**:
```c
struct eponMgr_controller_context {
    eponMgr_data_t *data;
    // ...
};

ctrl->data = malloc(sizeof(eponMgr_data_t));
eponMgr_data_init(ctrl->data, &config, ttl);
```

## Benefits of This Refactoring

1. **Clearer Intent**: Name accurately reflects the module's purpose
2. **Better Organization**: Core data structure now lives in `data_structures/` directory
3. **Improved Encapsulation**: Locking uses internal mutex, not controller's
4. **Consistency**: Variable naming (`eponData`) is more consistent across codebase
5. **Maintainability**: Easier for new developers to understand the architecture

## Testing

After refactoring, verify:
1. ✅ Code compiles without errors
2. ✅ All references updated consistently
3. ✅ Thread-safe access patterns preserved
4. ✅ No functional changes - pure refactoring

## Backward Compatibility

The old `hal_wrapper/` directory is **deprecated but preserved** for reference. It is no longer built or linked. A `DEPRECATED.md` file has been added with migration instructions.

In a future cleanup phase, these files will be removed entirely.

## Review Checklist

- [x] All `eponMgr_hal_wrapper` references updated
- [x] All function names updated
- [x] All variable names updated  
- [x] Thread-safety preserved
- [x] Build system updated
- [x] Public headers updated
- [x] Documentation created
- [x] Deprecation notices added

## Date

Refactoring completed: January 2, 2026
