# Building EPON Manager with Autotools

## Prerequisites

Install required build tools:

```bash
# On Red Hat/CentOS/Fedora
sudo yum install autoconf automake libtool gcc make

# On Debian/Ubuntu
sudo apt-get install autoconf automake libtool gcc make

# On macOS
brew install autoconf automake libtool
```

## Quick Start

```bash
# 1. Generate configure script
./autogen.sh

# 2. Configure the build
./configure

# 3. Build
make

# 4. Run tests (optional)
make check

# 5. Install (optional, requires root)
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

## Testing

```bash
# Run all tests
make check

# Run tests with verbose output
make check VERBOSE=1

# Run specific test
tests/unit/test_logger

# Run tests in specific directory
make -C tests/unit check
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
- **Libraries**: `$(libdir)/libepon_telemetry.so`, `$(libdir)/libeponMgr_logger.so`
- **Headers**: `$(includedir)/epon-manager/*.h`
- **Config**: `$(sysconfdir)/epon/`
- **Logs**: `$(localstatedir)/log/`

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

## Cross-Compilation

```bash
# For ARM target
./configure --host=arm-linux-gnueabihf \
    CC=arm-linux-gnueabihf-gcc \
    --prefix=/usr

# For 32-bit on 64-bit system
./configure CFLAGS="-m32" LDFLAGS="-m32"
```

## Parallel Builds

```bash
# Build with 4 parallel jobs
make -j4

# Use all available CPU cores
make -j$(nproc)
```

## Development Workflow

```bash
# After modifying configure.ac or Makefile.am
./autogen.sh
./configure [your options]
make

# After modifying source code only
make

# After git pull
make clean
./autogen.sh
./configure [your options]
make
```

## Troubleshooting

### "configure: command not found"

Run `./autogen.sh` first to generate the configure script.

### "aclocal: command not found"

Install autotools: `sudo yum install autoconf automake libtool`

### "libtool library used but 'LIBTOOL' is undefined"

Run `./autogen.sh` to regenerate build files.

### Build fails with "No rule to make target"

```bash
make distclean
./autogen.sh
./configure
make
```

### Tests fail to run

Make sure you configured with `--enable-tests` (default is yes):
```bash
./configure --enable-tests
make check
```

## Comparison with Old Build System

| Old Makefile | Autotools Equivalent |
|--------------|---------------------|
| `make` | `./autogen.sh && ./configure && make` |
| `./scripts/build_and_test.sh` | `make check` |
| `make clean` | `make clean` |
| Manual installation | `make install` |

## Configuration Variables

View all configuration variables:
```bash
./configure --help
```

Common variables:
- `CC` - C compiler
- `CFLAGS` - C compiler flags
- `LDFLAGS` - Linker flags
- `PKG_CONFIG_PATH` - Path to .pc files

Example:
```bash
./configure \
    CC=gcc \
    CFLAGS="-O3 -march=native" \
    LDFLAGS="-Wl,-rpath,/opt/lib"
```

## Support

For build issues, check:
1. All dependencies are installed
2. `./autogen.sh` completed without errors
3. `./configure` completed successfully
4. Check `config.log` for detailed error messages
