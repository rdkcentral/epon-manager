# epon-manager
The Rdk Epon Manager is an RDK application responsible for controlling and configuring EPON lower layers. It serves as a middleware between the EPON HAL and other RDK components.

## Quick Start

### Building and Testing

The project includes a comprehensive build and test script:

```bash
# Build everything and run all tests
./scripts/build_and_test.sh

# Clean build
./scripts/build_and_test.sh --clean

# Run only unit tests
./scripts/build_and_test.sh --unit-only

# Build without running tests
./scripts/build_and_test.sh --no-tests
```

See [scripts/BUILD_AND_TEST_GUIDE.md](scripts/BUILD_AND_TEST_GUIDE.md) for detailed documentation.

### Manual Build

If you prefer manual building:

```bash
# Build core application
cd src/core
make

# Build and run unit tests
cd tests/unit
make
make test
```

## Project Structure

```
epon-manager/
├── scripts/                   # Build and utility scripts
│   ├── build_and_test.sh     # Automated build and test script
│   └── BUILD_AND_TEST_GUIDE.md
├── src/
│   ├── logger/                # Logging subsystem
│   ├── rbus/                  # RBUS integration (Phase 6)
│   │   ├── dummy/            # Dummy RBUS for local testing
│   │   ├── wanmanager/       # WanManager PHY notifications
│   │   └── tr181/            # TR-181 handlers (Phase 7)
│   └── core/
│       ├── config/            # Configuration management
│       ├── data_structures/   # Core data structures
│       ├── hal_wrapper/       # HAL abstraction layer
│       ├── controller/        # Main controller logic
│       └── epon_manager_main.c
├── tests/
│   ├── hal_mock/              # Mock HAL for testing
│   ├── unit/                  # Unit tests
│   └── integration/           # Integration tests
├── docs/
│   ├── implementation/        # Implementation documentation
│   └── design_docs/          # Design documentation
└── include/                   # Public headers
    └── rbus/                 # RBUS dummy types
```

## Documentation

- [scripts/BUILD_AND_TEST_GUIDE.md](scripts/BUILD_AND_TEST_GUIDE.md) - Build and testing instructions
- [docs/implementation/IMPLEMENTATION_CHECKLIST.md](docs/implementation/IMPLEMENTATION_CHECKLIST.md) - Implementation progress
- [docs/implementation/](docs/implementation/) - Implementation documentation

## Features

- **Phase 1-4 Complete**: Core infrastructure, data structures, HAL wrapper
- **Phase 5 Complete**: Event-driven architecture with optimized condition variable signaling
- **Phase 6 Complete**: RBUS integration with dummy APIs for local testing
- Event queue for asynchronous event processing
- WanManager PHY status notifications
- Thread-safe data structures
- Comprehensive unit and integration tests (7 tests passing)
