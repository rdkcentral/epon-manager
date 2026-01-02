# EPON Manager Directory Structure

## Overview

This document defines the directory structure for the EPON Manager project, organized by functional groupings with separate libraries for modularity.

## Directory Tree

```
epon-manager/
├── src/
│   ├── core/                       # Core components (built together)
│   │   ├── controller/             # Main controller & initialization
│   │   ├── data_structures/        # Core data structures with HAL abstraction
│   │   ├── config/                 # Configuration management
│   │   └── stats_poller/           # Periodic statistics polling thread
│   │
│   ├── logger/                     # Logger wrapper (separate)
│   │   ├── epon_logger.c
│   │   ├── epon_logger.h
│   │   └── Makefile
│   │
│   ├── rbus/                       # RBUS library (libepon_rbus.so)
│   │   ├── tr181_handler/          # TR-181 DML handlers
│   │   ├── wanmanager_notify/      # WanManager notifications
│   │   ├── epon_rbus.c
│   │   ├── epon_rbus.h
│   │   └── Makefile
│   │
│   └── telemetry/                  # Telemetry library (libepon_telemetry.so)
│       ├── epon_telemetry.c
│       ├── epon_telemetry.h
│       └── Makefile
│
├── include/                        # Public headers
│   ├── epon_hal.h                  # EPON HAL interface (from rdkb-halif-epon)
│   ├── epon_manager.h              # Common manager definitions
│   ├── epon_manager_logger.h       # Logger API
│   ├── epon_manager_config.h       # Configuration API
│   ├── epon_manager_rbus.h         # RBUS API
│   └── epon_manager_telemetry.h    # Telemetry API
│
├── lib/                            # Built libraries
│   ├── libepon_hal_mock.so         # Mock HAL (for testing)
│   ├── libepon_rbus.so             # RBUS handler library
│   └── libepon_telemetry.so        # Telemetry library
│
├── tests/                          # Test suites
│   ├── hal_mock/                   # Mock HAL implementation
│   │   ├── epon_hal_mock.c         # Mock HAL implementation
│   │   ├── epon_hal_mock.h         # Mock HAL header
│   │   ├── test_hal_init.c         # Init tests
│   │   ├── test_hal_callbacks.c    # Callback tests
│   │   ├── test_hal_stats.c        # Stats query tests
│   │   ├── test_hal_events.c       # Event generation tests
│   │   └── Makefile
│   │
│   ├── unit/                       # Unit tests
│   │   ├── test_logger.c
│   │   ├── test_config.c
│   │   ├── test_data_structures.c
│   │   └── Makefile
│   │
│   ├── integration/                # Integration tests
│   │   ├── test_controller.c
│   │   ├── test_tr181_handlers.c
│   │   ├── test_alarm_handling.c
│   │   └── Makefile
│   │
│   └── system/                     # System tests
│       ├── test_startup.c
│       ├── test_shutdown.c
│       ├── test_wanmanager_integration.c
│       └── Makefile
│
├── build/                          # Build output (generated)
│   ├── obj/                        # Object files
│   └── bin/                        # Binaries
│
├── docs/                           # Documentation
│   ├── API.md                      # API documentation
│   ├── ARCHITECTURE.md             # Architecture overview
│   ├── BUILD.md                    # Build instructions
│   └── TESTING.md                  # Testing guide
│
├── design_docs/                    # Design documentation
│   ├── 01_Requirements.md
│   ├── 02_Architecture.md
│   ├── 03_Component_Design.md
│   ├── 04_Sequence_Diagrams.md
│   ├── 05_Thread_Architecture.md
│   └── 06_Configuration.md
│
├── .github/                        # GitHub configuration
│   └── prompts/
│       └── plan-eponManagerImplementationRevised.prompt.md
│
├── CMakeLists.txt                  # Root CMake file
├── Makefile                        # Root Makefile
├── README.md                       # Project README
├── EPON_HAL_Proposal.md            # HAL proposal document
├── IMPLEMENTATION_PLAN_SUMMARY.md  # Implementation plan summary
└── DIRECTORY_STRUCTURE.md          # This file
```

## Component Groupings

### 1. Core Components (`src/core/`)

These components are tightly coupled and built together:

- **controller/** - Main application controller
  - Initialization sequence
  - Thread management
  - Signal handling
  - Shutdown coordination

- **data_structures/** - Core data structures and HAL abstraction
  - Core data context (eponMgr_data_t)
  - HAL API wrappers
  - Simple timestamp-based caching
  - Cache management
  - Thread-safe HAL access
  - List management (LLIDs, CPEs, interfaces)

  - Cache implementation
  - Queue implementation
  - Thread-safe operations

- **config/** - Configuration management
  - INI file parsing
  - Environment variable support
  - Configuration validation

**Build Output:** Core components compiled into main executable

### 2. Logger Module (`src/logger/`)

Standalone logger wrapper that can be used by all components.

**Build Output:** Static library or compiled into main executable

### 3. RBUS Library (`src/rbus/`)

Separate library for RBUS/TR-181 functionality:

- **tr181_handler/** - TR-181 DML implementation
  - Parameter registration
  - GET handlers
  - SET handlers

- **wanmanager_notify/** - WanManager integration
  - PHY status notifications
  - Interface tracking
  - RBUS event publication

**Build Output:** `libepon_rbus.so` (links with `-lrbus` or dummy)

**Build Commands:**
```bash
# Dummy version (for local testing)
cd src/rbus
make DUMMY=1
# Output: ../../lib/libepon_rbus.so (with dummy RBUS stubs)

# Real version (for RDK)
cd src/rbus
make
# Output: ../../lib/libepon_rbus.so (links with -lrbus)
```

### 4. Telemetry Library (`src/telemetry/`)

Separate library for telemetry functionality:

- Event reporting
- Stats reporting
- Telemetry marker management

**Build Output:** `libepon_telemetry.so` (links with `-lt2` or dummy)

**Build Commands:**
```bash
# Dummy version (for local testing)
cd src/telemetry
make DUMMY=1
# Output: ../../lib/libepon_telemetry.so (with dummy T2 stubs)

# Real version (for RDK)
cd src/telemetry
make
# Output: ../../lib/libepon_telemetry.so (links with -lt2)
```

### 5. HAL Mock (`tests/hal_mock/`)

Mock HAL implementation for testing:

- Implements all APIs from `epon_hal.h`
- Event generation capabilities
- Test trigger functions (not in real HAL)
- Error injection

**Build Output:** `libepon_hal_mock.so`

**Build Commands:**
```bash
cd tests/hal_mock
make
# Output: ../../lib/libepon_hal_mock.so
```

## Build Order

1. **Logger** (static lib or compiled into main)
2. **HAL Mock** → `libepon_hal_mock.so`
3. **RBUS** → `libepon_rbus.so` (dummy first)
4. **Telemetry** → `libepon_telemetry.so` (dummy first)
5. **Core components** + Main executable

## Linking

Main executable links with:
```bash
gcc -o epon-manager epon_controller.o \
    -L./lib -lepon_hal_mock -lepon_rbus -lepon_telemetry \
    -L./src/core/hal_wrapper -lepon_hal_wrapper \
    -L./src/core/event_listener -lepon_event_listener \
    -L./src/core/data_structures -lepon_datastructures \
    -L./src/core/config -lepon_config \
    -L./src/logger -lepon_logger \
    -lpthread
```

## Testing Structure

### Unit Tests (`tests/unit/`)
- Test individual components
- Minimal dependencies
- Fast execution

### Integration Tests (`tests/integration/`)
- Test component interactions
- Use mock HAL
- Use dummy RBUS/Telemetry

### System Tests (`tests/system/`)
- End-to-end scenarios
- All components running
- Performance benchmarking

### Mock HAL Tests (`tests/hal_mock/`)
- Verify mock HAL correctness
- Test all callback types
- Error injection scenarios

## Development Workflow

### Phase 1: Logger (Week 1)
```
Working in: src/logger/
Testing:    tests/unit/test_logger.c
Output:     Logger working on Linux
```

### Phase 2: HAL Mock (Weeks 2-3)
```
Working in: tests/hal_mock/
Testing:    tests/hal_mock/test_*.c
Output:     lib/libepon_hal_mock.so + tests passing
```

### Phase 3: Core Infrastructure (Weeks 4-5)
```
Working in: src/core/config/, src/core/data_structures/
Testing:    tests/unit/test_config.c, test_cache.c, test_queue.c
Output:     Core data structures working
```

### Phase 4: Data Structures & Controller (Weeks 6-7)
```
Working in: src/core/data_structures/, src/core/controller/
Testing:    tests/unit/test_data_structures.c
Output:     Controller running with core data structures
```

### Phase 5: Stats Poller (Weeks 8-9)
```
Working in: src/core/stats_poller/
Testing:    tests/unit/test_stats_poller.c
Output:     Stats poller thread collecting HAL data
```

### Phase 6-7: RBUS (Weeks 10-13)
```
Working in: src/rbus/
Testing:    tests/unit/test_rbus.c
Output:     lib/libepon_rbus.so (dummy first, then full)
```

### Phase 8: Telemetry (Week 14)
```
Working in: src/telemetry/
Testing:    tests/unit/test_telemetry.c
Output:     lib/libepon_telemetry.so (dummy first)
```

### Phase 9: Integration Testing (Weeks 15-16)
```
Working in: tests/integration/
Testing:    All integration tests
Output:     Fully working system on Linux
```

### Phase 10: RDK Integration (Weeks 17-19)
```
Platform:   RDK-B
Testing:    On actual hardware
Output:     Production-ready
```

## File Naming Conventions

- C source files: `*.c`
- C header files: `*.h`
- Test files: `test_*.c`
- Makefiles: `Makefile`
- Documentation: `*.md`

## Code Organization

- Public APIs in `include/`
- Internal headers in `src/<module>/`
- One component per directory
- Clear separation of concerns
- Minimal dependencies between components

---

**Last Updated:** December 23, 2025  
**Status:** Structure defined, ready for implementation
