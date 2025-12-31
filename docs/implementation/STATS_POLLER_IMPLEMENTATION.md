# Stats Poller Implementation Summary

## Overview
Implemented FR2 requirement: periodic statistics polling thread (harvester) that queries HAL for statistics and updates the telemetry system.

## Implementation Date
December 26, 2024

## Files Created

### 1. **src/core/stats_poller/eponMgr_stats_poller.h**
- Header file defining stats poller API
- Structure: `eponMgr_stats_poller_t` with thread control, configuration, and HAL wrapper reference
- APIs:
  - `eponMgr_stats_poller_init()` - Initialize with HAL wrapper and config
  - `eponMgr_stats_poller_start()` - Start poller thread
  - `eponMgr_stats_poller_stop()` - Stop poller thread
  - `eponMgr_stats_poller_destroy()` - Cleanup resources
  - `eponMgr_stats_poller_is_running()` - Check running status
  - `eponMgr_stats_poller_set_enabled()` - Runtime enable/disable
  - `eponMgr_stats_poller_set_interval()` - Runtime interval change
  - `eponMgr_stats_poller_trigger_now()` - Trigger immediate collection

### 2. **src/core/stats_poller/eponMgr_stats_poller.c**
- Implementation file with thread function
- Thread behavior:
  - Sleeps using `pthread_cond_timedwait()` for efficient wake-up
  - Checks enabled flag before collecting
  - Collects link stats and transceiver stats via HAL wrapper
  - Logs collection results
  - TODO markers for telemetry integration (when implemented)
- Thread safety:
  - Mutex protects configuration changes
  - Condition variable enables immediate wake on trigger/shutdown
  - Clean shutdown handling

### 3. **src/core/stats_poller/Makefile.am**
- Autotools build configuration
- Builds `libeponMgr_stats_poller.la` static library
- Includes:
  - HAL wrapper headers
  - Logger headers
  - Top-level includes
- Links RBUS libraries

## Files Modified

### PSM Configuration

#### 1. **src/rbus/eponMgr_psm.h**
Added PSM keys:
```c
#define PSM_EPON_STATS_POLLER_ENABLED   "dmsb.eponmanager.StatsPollerEnabled"
#define PSM_EPON_STATS_POLLER_INTERVAL  "dmsb.eponmanager.StatsPollerIntervalSeconds"
```

### Persistence Layer

#### 2. **src/core/config/eponMgr_persistence.h**
Added to `eponMgr_persistence_t` structure:
```c
bool stats_poller_enabled;              /* Stats poller thread enabled (default: false) */
uint32_t stats_poller_interval_seconds; /* Stats poller interval in seconds (default: 900) */
```

#### 3. **src/core/config/eponMgr_persistence.c**
- **init_defaults()**: Set `stats_poller_enabled = false`, `stats_poller_interval_seconds = 900`
- **load()**: Read from `PSM_EPON_STATS_POLLER_ENABLED` and `PSM_EPON_STATS_POLLER_INTERVAL`
- **save()**: Write to `PSM_EPON_STATS_POLLER_ENABLED` and `PSM_EPON_STATS_POLLER_INTERVAL`
- **validate()**: Check interval is between 60-3600 seconds

### Controller Integration

#### 4. **src/core/controller/eponMgr_controller.c**
- Added `#include "eponMgr_stats_poller.h"`
- Added `eponMgr_stats_poller_t *stats_poller` to controller context structure
- **eponMgr_controller_init()**: 
  - Step 8: Initialize stats poller after HAL wrapper
  - Pass HAL wrapper reference and config settings
- **eponMgr_controller_run()**: 
  - Start stats poller thread if enabled in config
- **eponMgr_controller_destroy()**:
  - Stop and destroy stats poller before HAL wrapper cleanup
- **Error handling**: Added stats_poller cleanup to error path

### Build System

#### 5. **src/core/Makefile.am**
- Added `stats_poller` to `SUBDIRS`
- Added `$(top_builddir)/src/core/stats_poller/libeponMgr_stats_poller.a` to `epon_manager_LDADD`

#### 6. **configure.ac**
- Added `src/core/stats_poller/Makefile` to `AC_CONFIG_FILES`

## Configuration

### PSM Parameters

| Parameter | Type | Default | Range | Description |
|-----------|------|---------|-------|-------------|
| `dmsb.eponmanager.StatsPollerEnabled` | bool | false | true/false | Enable/disable stats poller thread |
| `dmsb.eponmanager.StatsPollerIntervalSeconds` | uint32 | 900 | 60-3600 | Polling interval in seconds |

### Default Behavior
- **Disabled by default**: Stats poller does not run unless explicitly enabled via PSM
- **15-minute interval**: When enabled, collects stats every 15 minutes (900 seconds)
- **Range validation**: Interval must be between 1 minute and 1 hour

## Runtime Control

The stats poller can be controlled at runtime through the API:

### Enable/Disable
```c
eponMgr_stats_poller_set_enabled(poller, true);  // Enable
eponMgr_stats_poller_set_enabled(poller, false); // Disable
```

### Change Interval
```c
eponMgr_stats_poller_set_interval(poller, 300); // Change to 5 minutes
```

### Trigger Immediate Collection
```c
eponMgr_stats_poller_trigger_now(poller); // Wake up and collect now
```

## Thread Architecture

### Sleep Mechanism
- Uses `pthread_cond_timedwait()` instead of busy-wait loop
- Wakes up on:
  1. Interval timeout (normal periodic wake)
  2. `trigger_now()` call (immediate collection)
  3. `stop()` call (shutdown)
  4. `set_interval()` call (apply new interval)

### Thread Safety
- All shared state protected by mutex
- Condition variable for efficient signaling
- Clean shutdown with join

## Statistics Collected

Currently collects two sets of statistics:

### 1. Link Statistics
- Bytes sent/received
- Packets sent/received
- Errors, drops, etc.
- Via `eponMgr_hal_wrapper_get_link_stats()`

### 2. Transceiver Statistics
- RX power (dBm)
- TX power (dBm)
- Temperature, voltage, etc.
- Via `eponMgr_hal_wrapper_get_transceiver_stats()`

## HAL Wrapper Integration

The stats poller:
1. Uses HAL wrapper reference passed during initialization
2. Calls HAL wrapper stats functions (respects cache TTL)
3. Does NOT directly call HAL - goes through wrapper layer
4. Benefits from thread-safe cache mechanism

## Telemetry Integration

### Current Status
- TODO markers in place for telemetry calls
- Structure ready for integration when telemetry system is implemented

### Planned Integration Points
```c
// In collect_all_stats() after successful collection:
eponMgr_telemetry_report_link_stats(&link_stats);
eponMgr_telemetry_report_transceiver_stats(&transceiver_stats);
```

## Testing

### Manual Testing
1. Enable stats poller via PSM:
   ```bash
   dmcli eRT setv dmsb.eponmanager.StatsPollerEnabled bool true
   dmcli eRT setv dmsb.eponmanager.StatsPollerIntervalSeconds uint 300
   ```

2. Restart epon_manager

3. Check logs for:
   ```
   Stats poller initialized (enabled: true, interval: 300s)
   Stats poller thread started
   Stats poller: Collected link stats (TX: ... bytes, RX: ... bytes)
   Stats poller: Collected transceiver stats (RX power: ... dBm, TX power: ... dBm)
   ```

### Expected Behavior
- If enabled: Thread starts, collects stats every interval
- If disabled: Thread does not start (no overhead)
- On shutdown: Thread stops cleanly, resources freed

## Build Instructions

After implementation:
```bash
./autogen.sh
./configure --enable-rbus --enable-tests
make clean
make
```

## Logging

All stats poller activities logged with appropriate levels:
- **INFO**: Thread start/stop, successful collection
- **DEBUG**: Skipped collection (disabled), early wake events
- **WARN**: Failed stats collection
- **ERROR**: Initialization failures

## Future Enhancements

1. **Telemetry Integration**: Connect to RDK telemetry system when available
2. **TR-181 Control**: Expose enable/disable and interval as TR-181 parameters
3. **Statistics Selection**: Allow configuration of which stats to collect
4. **Error Handling**: Implement retry logic for failed collections
5. **Performance Metrics**: Track collection timing and success rate

## Requirements Met

✅ **FR2: Stats Polling Thread (Harvester)**
- Optional thread (disabled by default)
- PSM configuration control
- Configurable interval (15-minute default)
- Queries HAL via wrapper
- Updates telemetry (placeholder ready)
- Thread-safe implementation
- Clean shutdown

## Dependencies

- **pthread**: Thread management, mutexes, condition variables
- **HAL wrapper**: Statistics queries with cache
- **PSM**: Persistent configuration storage
- **Logger**: Structured logging
- **RBUS**: (indirect, via HAL wrapper)

## Status

✅ **Implementation Complete**
- All files created and modified
- Build system updated
- Controller integration complete
- PSM configuration implemented
- Thread lifecycle managed
- Error handling in place

⏳ **Pending**
- Telemetry system integration (when available)
- Build and runtime testing
- TR-181 parameter exposure (future enhancement)
