# Building EPON Manager for RDK

## Overview

EPON Manager is built as part of the RDK build system using BitBake/Yocto. This document describes local development builds using autotools.

## Prerequisites

Install required build tools:

```bash
# On Red Hat/CentOS/Fedora
sudo yum install autoconf automake libtool gcc make

# On Debian/Ubuntu
sudo apt-get install autoconf automake libtool gcc make
```

## Quick Start

```bash
# 1. Generate configure script
./autogen.sh

# 2. Configure the build
./configure

# 3. Build
make

# 4. Install (optional, requires root)
sudo make install
```

## Configure Options

### Installation Paths

```bash
# Install to custom prefix (default: /usr/local)
./configure --prefix=/opt/epon-manager

# Common installation directories
./configure --prefix=/usr --sysconfdir=/etc --localstatedir=/var
```

### Feature Options

```bash
# Disable RBUS support
./configure --disable-rbus

# Disable telemetry support
./configure --disable-telemetry

# Disable building tests
./configure --disable-tests

# Enable debug build
./configure --enable-debug
```

### Combined Example

```bash
./configure \
    --prefix=/usr \
    --sysconfdir=/etc \
    --localstatedir=/var \
    --enable-debug
```

## Building

```bash
# Build everything
make

# Build with verbose output
make V=1

# Build specific component
make -C src/telemetry

# Clean build artifacts
make clean

# Remove all generated files (including configure)
make distclean
```

## Installation

```bash
# Install to configured prefix
sudo make install

# Uninstall
sudo make uninstall

# Install to staging directory (for packaging)
make install DESTDIR=/tmp/staging
```

### Installed Files

- **Binaries**: `$(prefix)/bin/epon_manager`
- **Libraries**: `$(libdir)/libepon_telemetry.so`, HAL mock: `$(libdir)/libepon_hal_mock.so`
- **Headers**: `$(includedir)/epon-manager/*.h`
- **Config**: `$(sysconfdir)/epon/`

## RDK Build Integration

In the RDK build environment, the component is built using BitBake:

```bash
# Clean and rebuild
bitbake -c cleanall rdkeponmanager
bitbake rdkeponmanager

# Deploy to target
# Output: /usr/bin/epon_manager
#         /usr/lib/libepon_hal_mock.so*
```

The BitBake recipe is at: `meta-rdk-wan/recipes-ccsp/ccsp/rdkeponmanager.bb`

## Distribution

```bash
# Create tarball for distribution
make dist

# This creates: epon-manager-1.0.0.tar.gz
```

## Cleaning

```bash
# Remove build artifacts (keep configure)
make clean

# Remove all generated files including configure
make distclean

# Remove distribution files
make maintainer-clean
```

## Configuration Variables

View all configuration variables:
```bash
./configure --help
```

Common variables:
- `CC` - C compiler
- `CFLAGS` - C compiler flags
- `LDFLAGS` - Linker flags

Example:
```bash
./configure \
    CC=gcc \
    CFLAGS="-O3 -march=native" \
    LDFLAGS="-Wl,-rpath,/opt/lib"
```
