# EPON Manager Implementation Checklist

Track progress through each phase of development.

## Phase 1: Logger Wrapper (Week 1)

- [ ] Create `src/logger/` directory structure
- [ ] Implement logger with console output
- [ ] Implement logger with file output
- [ ] Add log level filtering
- [ ] Add thread-safe mutex protection
- [ ] Create unit tests
- [ ] Test on Linux
- [ ] Verify no memory leaks (valgrind)
- [ ] Update documentation

**Status:** Not Started

---

## Phase 2: HAL Interface & Mock (Weeks 2-3)

- [x] Copy epon_hal.h to include/ directory
- [ ] Create `tests/hal_mock/` directory structure
- [ ] Implement all HAL initialization APIs
- [ ] Implement all HAL statistics APIs
- [ ] Implement all HAL information query APIs
- [ ] Implement callback registration
- [ ] Implement mock event trigger functions
- [ ] Create unit tests for mock
- [ ] Build as libepon_hal_mock.so
- [ ] Test all callbacks
- [ ] Verify thread safety
- [ ] Verify no memory leaks

**Status:** In Progress (epon_hal.h copied)

---

## Phase 3: Core Infrastructure (Weeks 4-5)

### Configuration Management
- [ ] Create `src/core/config/` directory
- [ ] Implement INI file parser
- [ ] Add environment variable support
- [ ] Add configuration validation
- [ ] Create unit tests
- [ ] Test on Linux

### Data Structures
- [ ] Create `src/core/data_structures/` directory
- [ ] Implement simple timestamp cache
- [ ] Implement thread-safe queue
- [ ] Create unit tests
- [ ] Test concurrent access
- [ ] Verify no memory leaks

**Status:** Not Started

---

## Phase 4: HAL Wrapper & Controller (Weeks 6-7)

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

**Status:** Not Started

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

**Completed Phases:** 0 / 10
**Current Phase:** Phase 1 - Logger Wrapper
**Overall Status:** 5% (epon_hal.h copied)

---

## Notes

- Original plan backup: `.github/prompts/plan-eponManagerImplementationRevised.prompt.md.backup`
- Design docs in: `design_docs/` directory
- References: EPON_HAL_Proposal.md, IMPLEMENTATION_PLAN_SUMMARY.md, DIRECTORY_STRUCTURE.md

---

**Last Updated:** December 23, 2025
