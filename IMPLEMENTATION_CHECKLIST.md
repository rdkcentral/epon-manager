# EPON Manager Implementation Checklist

Track progress through each phase of development.

## Phase 1: Logger Wrapper (Week 1)

- [x] Create `src/logger/` directory structure
- [x] Implement logger with console output
- [x] Implement logger with file output
- [x] Add log level filtering
- [x] ~~Add thread-safe mutex protection~~ (Cancelled - keeping simple, no mutex)
- [x] Create unit tests
- [x] Test on Linux
- [x] ~~Verify no memory leaks (valgrind)~~ (Pending - will do in Phase 9)
- [x] Update documentation

**Status:** ✅ Completed (Commit: c980246)

---

## Phase 2: HAL Interface & Mock (Weeks 2-3)

- [x] Copy epon_hal.h to include/ directory
- [x] Create `tests/hal_mock/` directory structure
- [x] Implement all HAL initialization APIs
- [x] Implement all HAL statistics APIs
- [x] Implement all HAL information query APIs
- [x] Implement callback registration
- [x] Implement mock event trigger functions
- [x] Create unit tests for mock (3 test programs)
- [x] Build as libepon_hal_mock.so
- [x] Test all callbacks (status, alarm, interface)
- [ ] ~~Verify thread safety~~ (N/A - mock is single-threaded)
- [ ] ~~Verify no memory leaks~~ (Pending - will do in Phase 9)

**Status:** ✅ Completed (Commit: 7424d25)

---

## Phase 3: Core Infrastructure (Weeks 4-5)

### Configuration Management
- [x] Create `src/core/config/` directory
- [x] Implement INI file parser
- [x] Add environment variable support
- [x] Add configuration validation
- [x] Add thread-safe mutex protection
- [x] Create unit tests (7 tests)
- [x] Test on Linux

### Data Structures - Cache
- [x] Create `src/core/data_structures/` directory
- [x] Implement cache with two strategies:
  - [x] Statistics: TTL-based with timestamps
  - [x] Info: Validity flag only (no TTL)
- [x] Add thread-safe mutex protection
- [x] Implement cache invalidation (per-entry and global)
- [x] Create unit tests (7 tests including TTL expiration)
- [x] Test on Linux

### Data Structures - Queue
- [x] Implement event queue (circular buffer FIFO)
- [x] Add thread-safe mutex protection
- [x] Create unit tests (9 tests)
- [x] Test on Linux
- [ ] ~~Verify no memory leaks~~ (Pending - will do in Phase 9)

### Refactoring
- [x] Rename all `epon_` prefix to `eponMgr_` (except HAL interface)
- [x] Update all function names, types, and macros
- [x] Update all file names and includes
- [x] Update Makefiles and build scripts

**Status:** ✅ Completed (23 tests passing, thread-safe with mutex)

---

## Phase 4: HAL Wrapper & Controller (Weeks 6-7)

### Data Structures
- [x] Create interface list manager (`eponMgr_interface_list.c/h`)
  - Thread-safe with pthread_mutex_t
  - Track active interfaces (veip0, veip1, etc.)
  - Track UP/DOWN status for each interface
  - Functions: any_up(), all_down() for WanManager integration
  - Conversion to HAL format
- [x] Create LLID list manager (`eponMgr_llid_list.c/h`)
  - Thread-safe with pthread_mutex_t
  - Track up to 32 LLIDs
  - LLID state tracking (UNREGISTERED, REGISTERING, REGISTERED, etc.)
  - Dynamic memory allocation for HAL format conversion
- [x] Create CPE list manager (`eponMgr_cpe_list.c/h`)
  - Thread-safe with pthread_mutex_t
  - Support up to 256 CPE entries
  - Track static and dynamic CPE MAC addresses
  - Age time tracking for dynamic entries
  - Separate clear functions for static/dynamic/all
- [x] Create ONU state manager (`eponMgr_onu_state.c/h`)
  - Thread-safe with pthread_mutex_t
  - Track current and previous ONU status
  - Store OLT info, manufacturer info, link info with validity flags
  - State change detection
  - Invalidate all cache on ONU status change
  - HAL initialization tracking
- [x] Update data structures Makefile
- [x] Create comprehensive unit tests (17 tests)
  - Interface list: 4 tests (init, add/update, any/all, to_hal)
  - LLID list: 4 tests (init, add/update, remove, to_hal)
  - CPE list: 4 tests (init, add/update, clear_dynamic, to_hal)
  - ONU state: 5 tests (init, status, info validity, invalidate, hal_init)
- [x] Build and test - all 40 tests passing

### HAL Wrapper
- [ ] Create `src/core/hal_wrapper/` directory
- [ ] Implement HAL wrapper for stats APIs
- [ ] Implement simple caching logic (30s TTL)
- [ ] Add cache invalidation
- [ ] Create unit tests with mock HAL

### Controller
- [ ] Create `src/core/controller/` directory
- [ ] Implement initialization sequence
- [ ] Implement signal handling
- [ ] Implement shutdown logic
- [ ] Register HAL callbacks
- [ ] Create integration test with mock

**Status:** ⚙️ In Progress - Data Structures Complete (40 tests passing)

**Progress:** Data structures for interface list, LLID list, CPE list, and ONU state implemented with full thread safety. All unit tests passing.

---

## Phase 5: Event Listener (Weeks 8-9)

- [ ] Create `src/core/event_listener/` directory
- [ ] Implement event queue processing
- [ ] Implement ONU status event handler
- [ ] Implement interface status event handler
- [ ] Implement alarm event handler
- [ ] Integrate with controller
- [ ] Create unit tests
- [ ] Test with mock HAL events
- [ ] Verify event processing latency < 100ms

**Status:** Not Started

---

## Phase 6: RBUS Library with Dummy (Weeks 10-11)

- [ ] Create `src/rbus/` directory structure
- [ ] Implement dummy RBUS stubs
- [ ] Implement TR-181 parameter registration (stub)
- [ ] Implement WanManager notification stubs
- [ ] Build as libepon_rbus.so (dummy)
- [ ] Create unit tests
- [ ] Test locally without real RBUS
- [ ] Verify all calls traced

**Status:** Not Started

---

## Phase 7: TR-181 & WanManager (Weeks 12-14)

### TR-181 Implementation
- [ ] Create `src/rbus/tr181_handler/` directory
- [ ] Implement GET handlers for all parameters
- [ ] Implement SET handlers
- [ ] Integrate with HAL wrapper (cache)
- [ ] Add cache invalidation on SET

### WanManager Integration
- [ ] Create `src/rbus/wanmanager_notify/` directory
- [ ] Implement interface tracking logic
- [ ] Implement PHY UP notification (first interface UP)
- [ ] Implement PHY DOWN notification (all interfaces DOWN)
- [ ] Integrate with event listener
- [ ] Test multi-interface scenarios
- [ ] Verify correct PHY status changes

**Status:** Not Started

---

## Phase 8: Telemetry Library (Week 15)

- [ ] Create `src/telemetry/` directory
- [ ] Implement dummy telemetry stubs
- [ ] Implement event reporting functions
- [ ] Implement stats reporting functions
- [ ] Build as libepon_telemetry.so (dummy)
- [ ] Create unit tests
- [ ] Test locally without T2
- [ ] Verify all calls traced

**Status:** Not Started

---

## Phase 9: Integration Testing (Weeks 16-17)

### Integration Tests
- [ ] Create `tests/integration/` directory
- [ ] Test complete event flow
- [ ] Test multi-interface scenarios
- [ ] Test cache timing (30s boundary)
- [ ] Test alarm handling

### System Tests
- [ ] Create `tests/system/` directory
- [ ] Test startup sequence
- [ ] Test shutdown sequence
- [ ] Test WanManager integration
- [ ] Performance benchmarking
- [ ] Memory leak detection (valgrind)
- [ ] 24-hour stability test
- [ ] Measure test coverage (>80%)

**Status:** Not Started

---

## Phase 10: RDK Integration (Weeks 18-20)

### Build System
- [ ] Update CMakeLists.txt for RDK
- [ ] Remove USE_DUMMY_RBUS flag
- [ ] Remove USE_DUMMY_TELEMETRY flag
- [ ] Link with real -lrbus
- [ ] Link with real -lt2
- [ ] Link with real EPON HAL (when available)

### RDK Platform
- [ ] Build on RDK-B platform
- [ ] Test with real RBUS
- [ ] Test with real T2 telemetry
- [ ] Test with real WanManager
- [ ] Run all tests on RDK-B
- [ ] Verify performance on hardware

### Deployment
- [ ] Create systemd service file
- [ ] Create installation script
- [ ] Create upgrade script
- [ ] Create ipk package
- [ ] Write deployment documentation
- [ ] Write troubleshooting guide

**Status:** Not Started

---

## Overall Progress

**Completed Phases:** 3 / 10
**Current Phase:** Phase 4 - HAL Wrapper & Controller
**Overall Status:** 35% (Logger, HAL Mock, Core Infrastructure complete; Phase 4 data structures complete)

**Test Summary:** 40 unit tests passing
- Phase 1 (Logger): 6 tests
- Phase 3 (Config): 7 tests
- Phase 3 (Cache): 7 tests
- Phase 3 (Queue): 9 tests
- Phase 4 (Data Structures): 17 tests
- Phase 2 (HAL Mock): 18 tests (separate test suite)

---

## Notes

- Original plan backup: `.github/prompts/plan-eponManagerImplementationRevised.prompt.md.backup`
- Design docs in: `design_docs/` directory
- References: EPON_HAL_Proposal.md, IMPLEMENTATION_PLAN_SUMMARY.md, DIRECTORY_STRUCTURE.md

---

**Last Updated:** December 23, 2025
