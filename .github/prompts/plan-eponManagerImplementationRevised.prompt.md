# EPON Manager Implementation Plan (Revised)

Incremental development plan for building the EPON Manager with local testing and modular architecture.

## Project Overview

**Objective:** Build RDK EPON Manager that integrates with WanManager via RBUS/TR-181

**Approach:** Component-by-component development with local testing before RDK integration

**Timeline:** 20 weeks (5 months)

---

## Directory Structure

```
epon-manager/
  src/
    core/                       # Core components (controller, hal_wrapper, event_listener, data_structures, config)
    logger/                     # Logger wrapper
    rbus/                       # RBUS/TR-181 library (separate .so)
    telemetry/                  # Telemetry library (separate .so)
  
  include/
    epon_hal.h                  # EPON HAL interface (from rdkb-halif-epon)
    epon_manager*.h             # Manager headers
  
  lib/                          # Built libraries (.so files)
  
  tests/
    hal_mock/                   # Mock HAL implementation
    unit/                       # Unit tests
    integration/                # Integration tests
    system/                     # System tests
```

See [DIRECTORY_STRUCTURE.md](../../../DIRECTORY_STRUCTURE.md) for complete structure.

---

## Build Strategy

Components built as separate shared libraries:

1. **libepon_hal_mock.so** - Mock HAL (tests/hal_mock/) for testing
2. **libepon_rbus.so** - RBUS/TR-181 handler (src/rbus/) with dummy option
3. **libepon_telemetry.so** - Telemetry wrapper (src/telemetry/) with dummy option
4. **epon-manager** - Main executable linking above libraries

**Local Development:** Use dummy implementations for RBUS and Telemetry
**RDK Integration:** Replace dummy with real implementations

---

## Development Phases

### Phase 1: Logger Wrapper (1 week)

**Location:** `src/logger/`

**Deliverables:**
- Logger implementation with console and file output
- Multi-level logging (FATAL, ERROR, WARN, INFO, DEBUG)
- Thread-safe operation
- Unit tests

**Success Criteria:**
- Compiles and runs on Linux
- All log levels work correctly
- Thread-safe verified
- No memory leaks

---

### Phase 2: HAL Interface & Mock (2 weeks)

**Location:** `tests/hal_mock/`, `include/epon_hal.h`

**Deliverables:**
- Copy epon_hal.h from rdkb-halif-epon ✅ DONE
- Complete mock HAL implementing all APIs from epon_hal.h
- Mock event trigger functions (for testing)
- Comprehensive test suite for mock

**Success Criteria:**
- Mock implements all HAL APIs
- Builds as libepon_hal_mock.so
- All callbacks work (status, alarm, interface)
- Can generate test events on demand
- All tests pass

---

### Phase 3: Core Infrastructure (2 weeks)

**Location:** `src/core/config/`, `src/core/data_structures/`

**Deliverables:**
- Configuration management (INI parser, env vars, validation)
- Simple timestamp-based cache structure
- Thread-safe event queue
- Unit tests for each

**Success Criteria:**
- Config loads and parses correctly
- Cache timestamp logic works (hit < 30s, miss > 30s)
- Queue is thread-safe
- All unit tests pass

---

### Phase 4: HAL Wrapper & Controller (2 weeks)

**Location:** `src/core/hal_wrapper/`, `src/core/controller/`

**Deliverables:**
- HAL wrapper with simple timestamp caching (30s TTL)
- Main controller with initialization sequence
- HAL callback registration
- Integration with mock HAL

**Success Criteria:**
- Cache hit/miss logic works
- HAL callbacks invoked by mock
- Controller initializes all components
- Runs standalone on Linux

---

### Phase 5: Event Listener (2 weeks)

**Location:** `src/core/event_listener/`

**Deliverables:**
- Event listener thread
- Event queue processing
- Event handlers for: ONU status, Interface status, Alarms
- Integration with mock HAL callbacks

**Key Logic:**
- **ONU status events:** Update internal state only (no WanManager notification)
- **Interface status events:** Track interfaces and notify WanManager (PHY UP/DOWN)
- **Alarm events:** Log and telemetry

**Success Criteria:**
- Events queued and processed in order
- Event processing latency < 100ms
- Thread-safe operation
- Mock can trigger events

---

### Phase 6: RBUS Library with Dummy (2 weeks)

**Location:** `src/rbus/`

**Deliverables:**
- RBUS library with dual implementation (dummy/real)
- TR-181 parameter handlers (stubs initially)
- WanManager notification stubs
- Builds as libepon_rbus.so

**Real RBUS Implementation:**
- Uses real RBUS library from rdkcentral/rbus
- Full RBUS API integration
- All function calls use real RBUS operations

**Success Criteria:**
- Compiles with dummy implementation
- Can test locally without real RBUS
- All function calls traced
- Ready for real RBUS integration

---

### Phase 7: TR-181 & WanManager Integration (3 weeks)

**Location:** `src/rbus/tr181_handler/`, `src/rbus/wanmanager_notify/`

**Deliverables:**
- Complete TR-181 parameter registration
- GET handlers using HAL wrapper (with cache)
- SET handlers with cache invalidation
- WanManager PHY status notification logic
- Interface tracking for multi-interface support

**WanManager Logic:**
- **PHY UP:** When ANY interface goes UP (first interface)
- **PHY DOWN:** When ALL interfaces go DOWN (last interface)

**Success Criteria:**
- All TR-181 parameters registered
- GET returns correct cached values
- PHY UP/DOWN notifications work correctly
- Can test with dummy RBUS locally

---

### Phase 8: Telemetry Library with Dummy (1 week)

**Location:** `src/telemetry/`

**Deliverables:**
- Telemetry library with dual implementation (dummy/real)
- Event reporting functions
- Stats reporting functions
- Builds as libepon_telemetry.so

**Dummy Implementation:**
- Printf-based stubs for local testing
- Controlled by USE_DUMMY_TELEMETRY flag

**Success Criteria:**
- Compiles with dummy implementation
- Can test locally without T2
- All calls traced

---

### Phase 9: Local Integration Testing (2 weeks)

**Location:** `tests/integration/`, `tests/system/`

**Test Scenarios:**
1. Complete event flow (ONU registration → Interface UP → WanManager notify)
2. Multi-interface PHY status (veip0, veip1 UP/DOWN sequences)
3. Cache timing validation (hit/miss at 30s boundary)
4. Alarm handling end-to-end
5. Performance benchmarking
6. Memory leak detection (valgrind)
7. 24-hour stability test

**Deliverables:**
- Complete integration test suite
- Performance benchmarks
- Memory leak reports
- Coverage reports (>80%)

**Success Criteria:**
- All tests pass on Linux
- Event processing < 100ms
- TR-181 query < 10ms (cached)
- No memory leaks
- No race conditions
- System stable over 24 hours

---

### Phase 10: RDK Platform Integration (3 weeks)

**Location:** All components

**Tasks:**
1. Update build for RDK environment
2. ✅ Removed USE_DUMMY_RBUS flag (using real RBUS)
3. Remove USE_DUMMY_TELEMETRY flag
4. ✅ Link with real -lrbus library
5. Link with real -lt2 libraries
6. Replace mock HAL with real HAL (when available)
7. Test on actual RDK-B platform
8. Create systemd service configuration
9. Package for deployment (ipk)

**Deliverables:**
- RDK-B compatible build
- systemd service file
- Installation/upgrade scripts
- Complete documentation
- Production-ready binary

**Success Criteria:**
- Service starts on boot
- All tests pass on RDK-B
- Integration with real WanManager confirmed
- Performance targets met on hardware
- Ready for production deployment

---

## Key Implementation Rules

### Event Processing

1. **ONU Status Events** (`onu_status_callback`)
   - Purpose: Internal state tracking ONLY
   - Actions: Update cache, log, telemetry
   - **DO NOT notify WanManager**

2. **Interface Status Events** (`interface_status_callback`)
   - Purpose: Per-interface tracking and WanManager integration
   - **PRIMARY mechanism for WanManager updates**
   - Track active interface list
   - Notify WanManager on PHY status changes

### WanManager Integration

- **PHY UP:** When ANY interface goes UP (first interface)
- **PHY DOWN:** When ALL interfaces go DOWN (last interface)
- Always include interface name in notifications
- Use RBUS events for communication

### Cache Strategy

- Simple timestamp-based: `time(NULL) - last_query_time > TTL`
- Default TTL: 30 seconds (configurable)
- Thread-safe with pthread_mutex
- Invalidate on SET operations (reset timestamp to 0)
- No complex algorithms - keep it simple

### Thread Safety

- Lock ordering: Config → Cache → Queue (never reverse)
- Use pthread_mutex_timedlock with 5s timeout
- No blocking operations while holding locks
- Always unlock on error paths

---

## Testing Strategy

**Unit Tests:** Each component tested individually on Linux

**Integration Tests:** Component interactions with mock HAL and dummy RBUS/Telemetry

**System Tests:** End-to-end scenarios, performance, stress testing

**Mock HAL Tests:** Comprehensive coverage of all HAL APIs and callbacks

---

## Dependencies

**For Local Development (Phases 1-9):**
- GCC compiler, pthread, standard C libraries
- CMake or Make
- Mock HAL (our implementation)
- Optional: valgrind, gdb, test frameworks

**For RDK Platform (Phase 10):**
- EPON HAL library (when available)
- RBUS/DBus libraries
- RDK Logger, T2 Telemetry
- RDK-B build environment

---

## Timeline Summary

| Phase | Component | Duration | Environment |
|-------|-----------|----------|-------------|
| 1 | Logger Wrapper | 1 week | Linux |
| 2 | HAL Interface & Mock | 2 weeks | Linux |
| 3 | Core Infrastructure | 2 weeks | Linux |
| 4 | HAL Wrapper & Controller | 2 weeks | Linux |
| 5 | Event Listener | 2 weeks | Linux |
| 6 | RBUS Library (Dummy) | 2 weeks | Linux |
| 7 | TR-181 & WanManager | 3 weeks | Linux |
| 8 | Telemetry Library (Dummy) | 1 week | Linux |
| 9 | Local Integration Testing | 2 weeks | Linux |
| 10 | RDK Platform Integration | 3 weeks | RDK-B |
| **Total** | | **20 weeks** | |

---

## Success Metrics

### Functional
- ✅ All functional requirements implemented
- ✅ WanManager integration working
- ✅ TR-181 DML complete

### Non-Functional
- ✅ Event processing < 100ms
- ✅ Stats query < 10ms (cached), < 200ms (HAL)
- ✅ Cache hit rate > 90%
- ✅ Memory < 20MB
- ✅ CPU overhead < 10%
- ✅ Thread-safe operations
- ✅ No memory leaks
- ✅ Test coverage > 80%

---

## References

- Design Documents: `design_docs/` directory
- EPON HAL Proposal: `EPON_HAL_Proposal.md`
- EPON HAL Interface: `include/epon_hal.h`
- Directory Structure: `DIRECTORY_STRUCTURE.md`
- Implementation Summary: `IMPLEMENTATION_PLAN_SUMMARY.md`

---

**Plan Version:** 2.0  
**Last Updated:** December 23, 2025  
**Status:** Ready for Phase 1 implementation
