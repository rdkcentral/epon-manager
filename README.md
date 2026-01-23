# epon-manager

The RDK EPON Manager is an RDK application responsible for controlling and configuring EPON lower layers. It serves as middleware between the EPON HAL and other RDK components.

## Building

### RDK Yocto Build (Production)

```bash
# Clean and rebuild
bitbake -c cleanall rdk-eponmanager
bitbake rdk-eponmanager

# Output:
#   /usr/bin/epon_manager
#   /usr/lib/libepon_hal_mock.so (if tests enabled)
```

### Autotools Build (Development)

```bash
# 1. Generate configure script (first time only)
autoreconf -ivf

# 2. Configure with options
./configure [OPTIONS]

# 3. Build
make -j$(nproc)

# 4. Install (optional)
sudo make install

# 5. Clean (if needed)
make clean
```

### Configure Options

```bash
# Installation paths
./configure --prefix=/usr \
            --sysconfdir=/etc \
            --localstatedir=/var

# Feature toggles
./configure --enable-rbus           # Enable RBUS support (default: yes)
./configure --disable-rbus          # Disable RBUS support
./configure --enable-telemetry      # Enable telemetry (default: yes)
./configure --disable-telemetry     # Disable telemetry
./configure --enable-tests          # Build HAL mock library (default: yes)
./configure --disable-tests         # Don't build HAL mock
./configure --enable-debug          # Debug build with -g -O0 (default: no)

# Complete development build example
./configure --prefix=/usr \
            --enable-rbus \
            --enable-telemetry \
            --enable-tests \
            --enable-debug

# Production build example
./configure --prefix=/usr \
            --sysconfdir=/etc \
            --enable-rbus \
            --enable-telemetry \
            --disable-tests
```

## Project Structure

```
epon-manager/
├── include/                   # Public headers
│   ├── epon_hal.h            # EPON HAL interface
│   ├── eponMgr_rbus.h        # RBUS public API
│   ├── eponMgr_telemetry.h   # Telemetry public API
│   └── eponMgr_tr181.h       # TR-181 public API
├── src/
│   ├── logger/               # RDK Logger integration (header-only)
│   │   └── eponMgr_logger.h
│   ├── core/                 # Core manager components
│   │   ├── epon_manager_main.c      # Main entry point
│   │   ├── controller/              # Main controller logic
│   │   ├── data_structures/         # Core data structures & HAL abstraction
│   │   ├── stats_poller/            # Statistics polling thread
│   │   └── config/                  # Persistence & configuration
│   ├── rbus/                 # RBUS integration
│   │   ├── eponMgr_rbus.c           # RBUS initialization
│   │   ├── eponMgr_psm.c/h          # PSM (Persistent Storage Manager)
│   │   ├── tr181/                   # TR-181 data model handlers
│   │   │   └── eponMgr_tr181.c
│   │   └── wanmanager_update/       # WanManager PHY notifications
│   └── telemetry/            # Telemetry subsystem
│       └── eponMgr_telemetry.c
├── tests/
│   └── hal_mock/             # Mock EPON HAL for testing
│       ├── epon_hal_mock.c   # Mock implementation
│       └── epon_hal_trigger.c # Trigger utility
├── systemd/                  # Systemd service files
│   └── utils/
│       └── rdkeponmanager.service
├── design_docs/              # Architecture documentation
│   ├── 01_Requirements.md
│   ├── 02_Architecture.md
│   ├── 03_Component_Design.md
│   ├── 04_Sequence_Diagrams.md
│   ├── 05_Thread_Architecture.md
│   └── 06_Configuration.md
├── configure.ac              # Autoconf configuration
├── Makefile.am              # Automake top-level
└── cfg/                     # Autotools auxiliary files
```

## Features

- **RBUS Integration**: TR-181 data model support (Device.Optical.Interface.{i})
- **PSM Persistence**: Configuration storage via Persistent Storage Manager
- **WanManager Integration**: PHY status notifications for link state
- **Statistics Polling**: Configurable periodic statistics collection
- **RDK Logger**: Integrated logging with RDK_LOG macros
- **Telemetry**: Event reporting and statistics collection
- **HAL Abstraction**: Clean separation between manager and HAL
- **Thread-Safe**: Event-driven architecture with mutex/condition variables
- **Multi-LLID Support**: Dynamic LLID table management
- **DPoE Support**: CPE MAC table and VEIP interface management
