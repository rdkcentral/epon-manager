# EPON Manager Implementation Plan - Summary of Changes

## Date: December 23, 2025

## Overview

This document summarizes the revised implementation plan for the EPON Manager based on the actual EPON HAL interface (`epon_hal.h`) from rdkb-halif-epon repository.

## Key Changes from Original Plan

### 1. Directory Structure - Modular Organization

**New Structure:**
```
epon-manager/
  src/
    core/                       # Core components together
      controller/               # Main controller
      hal_wrapper/              # HAL wrapper with cache
      event_listener/           # Event listener thread
      data_structures/          # Queues, caches
      config/                   # Configuration management
    
    logger/                     # Logger wrapper (separate)
    
    rbus/                       # RBUS/TR-181 (separate library - libepon_rbus.so)
      tr181_handler/            # TR-181 DML implementation
      wanmanager_notify/        # WanManager notifications
    
    telemetry/                  # Telemetry (separate library - libepon_telemetry.so)
      telemetry_wrapper/        # T2 telemetry wrapper
  
  include/
    epon_hal.h                  # EPON HAL interface (copied from rdkb-halif-epon)
    epon_manager.h              # Manager common headers
    epon_manager_logger.h       # Logger API
    epon_manager_config.h       # Config API
    epon_manager_rbus.h         # RBUS API
    epon_manager_telemetry.h    # Telemetry API
  
  lib/                          # Built libraries
    libepon_hal_mock.so         # Mock HAL library
    libepon_rbus.so             # RBUS handler library
    libepon_telemetry.so        # Telemetry library
  
  tests/
    hal_mock/                   # Mock HAL implementation & tests
    unit/                       # Unit tests per component
    integration/                # Integration tests
```

### 2. Build Strategy - Separate Libraries

**Components built as shared libraries:**

1. **libepon_hal_mock.so** - Mock HAL for testing
   - Location: `tests/hal_mock/`
   - Implements all APIs from `epon_hal.h`
   - Provides test trigger functions

2. **libepon_rbus.so** - RBUS/TR-181 handler
   - Location: `src/rbus/`
   - Links with `-lrbus` (or dummy for local testing)
   - TR-181 parameter handlers
   - WanManager notification logic

3. **libepon_telemetry.so** - Telemetry wrapper
   - Location: `src/telemetry/`
   - Links with `-lt2` (or dummy for local testing)
   - Event and stats reporting

4. **epon-manager** - Main executable
   - Links with above libraries via `-l` flags
   - Core components compiled in

**Build Command Example:**
```bash
gcc -o epon-manager epon_controller.c \
    -I../../../include \
    -L../../../lib -lepon_hal_mock -lepon_rbus -lepon_telemetry \
    -L../hal_wrapper -lepon_hal_wrapper \
    -L../event_listener -lepon_event_listener \
    -L../data_structures -lepon_datastructures \
    -L../config -lepon_config \
    -L../../logger -lepon_logger \
    -lpthread
```

### 3. Development Approach - Incremental & Local

**Key Principle: Build and test each component locally before integration**

**Phase-by-Phase Approach:**

**Phase 1 (1 week):** Logger Wrapper
- Implement logger with console/file output
- Build as static library or compile into main
- Test locally with unit tests
- **Output:** Logger working on Linux

**Phase 2 (2 weeks):** HAL Interface & Mock
- Copy `epon_hal.h` from rdkb-halif-epon
- Implement complete mock HAL as shared library
- Create comprehensive test suite
- **Output:** `libepon_hal_mock.so` + tests passing

**Phase 3 (2 weeks):** Core Infrastructure
- Config management
- Data structures (cache, queue)
- Build and test each separately
- **Output:** Core infrastructure working locally

**Phase 4 (2 weeks):** HAL Wrapper & Controller
- HAL wrapper with timestamp cache
- Main controller
- Integration with mock HAL
- **Output:** Controller + HAL wrapper + mock running locally

**Phase 5 (2 weeks):** Event Listener
- Event processing thread
- Integration with mock callbacks
- **Output:** Events flowing from mock → listener → processing

**Phase 6 (2 weeks):** RBUS Library (Dummy)
- Build as `libepon_rbus.so`
- Dummy implementation for local testing
- TR-181 handlers (compile but don't execute fully)
- **Output:** RBUS library compiles and links

**Phase 7 (3 weeks):** RBUS/TR-181 Full Implementation
- Complete TR-181 parameter handlers
- WanManager notification logic
- Integration with event listener
- **Output:** Full RBUS integration working with dummy backend

**Phase 8 (1 week):** Telemetry Library (Dummy)
- Build as `libepon_telemetry.so`
- Dummy implementation for local testing
- **Output:** Telemetry library compiles and links

**Phase 9 (2 weeks):** Local Integration Testing
- End-to-end tests with all components
- Performance benchmarking
- Memory leak detection
- **Output:** Fully working system on Linux

**Phase 10 (3 weeks):** RDK Platform Integration
- Replace dummy RBUS with real
- Replace dummy telemetry with real
- Replace mock HAL with real (when available)
- **Output:** Production-ready on RDK-B

### 4. HAL Interface - Use Actual Structures

**All implementations now use structs from `epon_hal.h`:**

From the actual header:
- `epon_hal_config_t` - HAL configuration with callbacks
- `epon_hal_link_stats_t` - Link statistics
- `epon_hal_transceiver_stats_t` - Transceiver stats
- `epon_onu_manufacturer_info_t` - Manufacturer info
- `epon_hal_link_info_t` - Link information
- `epon_interface_list_t` - Interface list
- `epon_llid_list_t` - LLID information
- `epon_olt_info_t` - OLT information
- `epon_onu_status_t` - ONU status enum
- `epon_interface_link_status_t` - Interface status enum
- `epon_hal_alarm_t` - Alarm types enum
- `epon_hal_return_t` - Return codes enum

**HAL Wrapper Example:**
```c
// Uses actual structures from epon_hal.h
int epon_hal_wrapper_get_link_stats(epon_hal_link_stats_t *stats) {
    // Check cache validity
    if (epon_cache_is_valid(&g_wrapper.link_stats_cache)) {
        return epon_cache_get(&g_wrapper.link_stats_cache, stats);
    }
    
    // Cache expired - query HAL
    stats->struct_size = sizeof(epon_hal_link_stats_t);
    epon_hal_return_t ret = epon_hal_get_link_stats(stats);
    
    if (ret == EPON_HAL_SUCCESS) {
        epon_cache_update(&g_wrapper.link_stats_cache, stats);
    }
    
    return (ret == EPON_HAL_SUCCESS) ? 0 : -1;
}
```

### 5. Real RBUS Implementation

**RBUS Integration:**
```c
#include <rbus/rbus.h>

int rbus_open(rbusHandle_t* handle, const char* component_name) {
    // Real RBUS library call
    return rbus_open(handle, component_name);
}

int rbus_regDataElements(rbusHandle_t handle, int numElements, 
                         rbusDataElement_t* elements) {
    printf("[DUMMY_RBUS] Registered %d data elements\n", numElements);
    return 0;
}
// ... other dummy implementations
#else
#include <rbus.h>  // Real RBUS for RDK builds
#endif
```

**Telemetry Dummy Implementation:**
```c
#define USE_DUMMY_TELEMETRY  // For local builds

#ifdef USE_DUMMY_TELEMETRY
int t2_event_s(const char* marker, const char* value) {
    printf("[DUMMY_TELEMETRY] %s=%s\n", marker, value);
    return 0;
}

int t2_event_d(const char* marker, uint32_t value) {
    printf("[DUMMY_TELEMETRY] %s=%u\n", marker, value);
    return 0;
}
#else
#include <telemetry_busmessage_sender.h>  // Real T2 for RDK builds
#endif
```

**Benefits:**
- Can compile and test locally without RDK dependencies
- All function calls traced via printf for debugging
- Easy to switch to real implementation (remove #define)
- Same API, just different backend

### 6. Testing Strategy - Local First

**Unit Tests (tests/unit/):**
- Each component tested individually
- Compile and run on Linux
- Use mock HAL for HAL-dependent tests
- Example:
  ```bash
  gcc test_hal_wrapper.c -o test_hal_wrapper \
      -I../../include \
      -L../../lib -lepon_hal_mock \
      -lpthread
  ./test_hal_wrapper
  ```

**Integration Tests (tests/integration/):**
- Test component interactions
- Use mock HAL to trigger events
- Use dummy RBUS/telemetry
- Example:
  ```bash
  # Start epon-manager with dummy libs
  ./epon-manager --config test_config.ini &
  
  # Trigger mock events
  mock_hal_trigger_interface_status("veip0", EPON_ONU_INTF_STATUS_LINK_UP)
  
  # Check output
  # Expected: [DUMMY_WANMANAGER] PHY Status: UP
  ```

**System Tests (tests/system/):**
- Full end-to-end scenarios
- All components running
- Performance benchmarking
- Memory leak detection (valgrind)

### 7. Timeline Summary

| Phase | Component | Duration | Environment |
|-------|-----------|----------|-------------|
| 1 | Logger Wrapper | 1 week | Linux |
| 2 | HAL Interface & Mock | 2 weeks | Linux |
| 3 | Core Infrastructure | 2 weeks | Linux |
| 4 | HAL Wrapper & Controller | 2 weeks | Linux |
| 5 | Event Listener | 2 weeks | Linux |
| 6 | RBUS Library (Dummy) | 2 weeks | Linux |
| 7 | RBUS/TR-181 Full | 3 weeks | Linux |
| 8 | Telemetry Library (Dummy) | 1 week | Linux |
| 9 | Local Integration Testing | 2 weeks | Linux |
| 10 | RDK Platform Integration | 3 weeks | RDK-B |
| **Total** | **All Components** | **20 weeks** | |

### 8. Key Benefits of New Approach

1. **Incremental Progress:**
   - Build one component at a time
   - Test immediately after building
   - Catch issues early

2. **Local Development:**
   - Don't need RDK platform until Phase 10
   - Don't need real RBUS/Telemetry until Phase 10
   - Faster development cycle

3. **Modular Libraries:**
   - RBUS as separate .so file
   - Telemetry as separate .so file
   - Easy to replace dummy with real
   - Clear dependencies

4. **Better Testing:**
   - Test with mock HAL throughout
   - Dummy RBUS/Telemetry for local testing
   - Integration tests run on Linux
   - RDK platform only for final validation

5. **Risk Mitigation:**
   - Mock HAL developed early (Phase 2)
   - Not blocked by HAL availability
   - Not blocked by RDK platform access
   - Can demonstrate progress incrementally

## Next Steps

1. **Copy epon_hal.h** ✅ DONE
   - Copied to `include/epon_hal.h`

2. **Start Phase 1: Logger Wrapper**
   - Implement logger based on HAL_LOG macro style
   - Create unit tests
   - Build and test locally

3. **Plan Phase 2: Mock HAL**
   - Design mock API based on epon_hal.h
   - Plan test scenarios
   - Set up test infrastructure

## Files Modified/Created

- ✅ `/include/epon_hal.h` - Copied from rdkb-halif-epon
- ✅ `.github/prompts/plan-eponManagerImplementationRevised.prompt.md` - Updated plan
- ✅ `IMPLEMENTATION_PLAN_SUMMARY.md` - This summary document

## References

- Original plan: `.github/prompts/plan-eponManagerImplementationRevised.prompt.md`
- EPON HAL Proposal: `EPON_HAL_Proposal.md`
- EPON HAL Header: `include/epon_hal.h` (from rdkb-halif-epon)
- Design docs: `design_docs/` directory

---

**Plan revised on:** December 23, 2025  
**Revised by:** Development Team  
**Status:** Ready to start Phase 1 implementation
