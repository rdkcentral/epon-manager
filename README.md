# epon-manager

The RDK EPON Manager is an RDK application responsible for controlling and configuring EPON lower layers. It serves as middleware between the EPON HAL and other RDK components.

## Building

### RDK Build (Production)

```bash
# Clean and rebuild
bitbake -c cleanall rdkeponmanager
bitbake rdkeponmanager

# Output:
#   /usr/bin/epon_manager
#   /usr/lib/libepon_hal_mock.so*
```

### Local Development Build

```bash
# 1. Generate configure script (first time only)
./autogen.sh

# 2. Configure
./configure

# 3. Build
make

# 4. Install (optional)
sudo make install
```

See [BUILDING.md](BUILDING.md) for detailed build documentation.

### Configure Options

```bash
# Standard installation paths
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var

# Feature options
./configure --enable-debug        # Debug build
./configure --disable-rbus        # Disable RBUS support
./configure --disable-telemetry   # Disable telemetry
./configure --disable-tests       # Don't build HAL mock
```

## Project Structure

```
epon-manager/
├── src/
│   ├── logger/                # RDK Logger integration (header-only)
│   ├── rbus/                  # RBUS integration
│   │   ├── wanmanager/       # WanManager PHY notifications
│   │   └── tr181/            # TR-181 data model handlers
│   ├── telemetry/             # Telemetry subsystem
│   └── core/
│       ├── config/            # Configuration management
│       ├── data_structures/   # Core data structures (includes HAL abstraction)
│       ├── controller/        # Main controller logic
│       └── epon_manager_main.c
├── tests/
│   └── hal_mock/              # Mock EPON HAL for integration testing
├── include/                   # Public headers (epon_hal.h)
└── docs/                      # Documentation
```

## Features

- **RBUS Integration**: TR-181 data model support (Device.Optical.Interface)
- **WanManager Integration**: PHY status notifications
- **RDK Logger**: Integrated logging with RDK_LOG macros
- **Telemetry**: Event reporting and statistics collection
- **HAL Abstraction**: Clean separation between manager and HAL
- **Thread-Safe**: Event-driven architecture with condition variables
