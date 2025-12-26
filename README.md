# epon-manager
The Rdk Epon Manager is an RDK application responsible for controlling and configuring EPON lower layers. It serves as a middleware between the EPON HAL and other RDK components.

## Quick Start

### Building with Autotools (Recommended)

```bash
# 1. Generate configure script (first time only)
./autogen.sh

# 2. Configure
./configure

# 3. Build
make

# 4. Run tests
make check

# 5. Install (optional)
sudo make install
```

See [BUILDING.md](BUILDING.md) for detailed autotools documentation.

### Alternative: Legacy Build Script

The project still includes the original build script:

```bash
# Build everything and run all tests
./scripts/build_and_test.sh

# Clean build
./scripts/build_and_test.sh --clean

# Run only unit tests
./scripts/build_and_test.sh --unit-only
```

See [scripts/BUILD_AND_TEST_GUIDE.md](scripts/BUILD_AND_TEST_GUIDE.md) for detailed documentation.

## Build System

EPON Manager uses GNU Autotools for building:
- **configure.ac** - Autoconf configuration
- **Makefile.am** - Automake makefiles
- **autogen.sh** - Bootstrap script

### Configure Options

```bash
# Standard installation paths
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var

# Feature options
./configure --enable-debug        # Debug build
./configure --disable-rbus        # Disable RBUS support
./configure --disable-telemetry   # Disable telemetry
./configure --disable-tests       # Don't build tests
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
