#!/bin/sh
# autogen.sh - Generate configure script and Makefiles for EPON Manager
# Run this script to bootstrap the autotools build system

set -e

# Check for required tools
echo "Checking for required autotools..."

MISSING_TOOLS=""
for tool in autoconf automake; do
    if ! command -v $tool >/dev/null 2>&1; then
        MISSING_TOOLS="$MISSING_TOOLS $tool"
    fi
done

# Check for libtool/libtoolize (different names on different systems)
if ! command -v libtoolize >/dev/null 2>&1 && ! command -v glibtoolize >/dev/null 2>&1; then
    MISSING_TOOLS="$MISSING_TOOLS libtool"
fi

if [ -n "$MISSING_TOOLS" ]; then
    echo "ERROR: Missing tools:$MISSING_TOOLS"
    echo "Please install autotools: autoconf automake libtool"
    exit 1
fi

# Determine which libtool command to use
if command -v libtoolize >/dev/null 2>&1; then
    LIBTOOLIZE=libtoolize
else
    LIBTOOLIZE=glibtoolize
fi

echo "All required tools found"
echo ""

# Create m4 directory if it doesn't exist
echo "Creating m4 directory..."
mkdir -p m4

# Create cfg directory for auxiliary files if it doesn't exist
echo "Creating cfg directory..."
mkdir -p cfg

echo "Running aclocal..."
aclocal -I m4

echo "Running autoheader..."
autoheader

echo "Running $LIBTOOLIZE..."
$LIBTOOLIZE --force --copy --automake

echo "Running automake..."
automake --add-missing --copy --force-missing --foreign

echo "Running autoconf..."
autoconf

echo ""
echo "=========================================="
echo "Autotools setup complete!"
echo "=========================================="
echo ""
echo "You can now build the project:"
echo "  ./configure [OPTIONS]"
echo "  make"
echo "  make check    # Run tests"
echo "  make install  # Install (as root)"
echo ""
echo "Useful configure options:"
echo "  --prefix=PREFIX         Install to PREFIX (default: /usr/local)"
echo "  --enable-debug          Enable debug build"
echo "  --disable-rbus          Disable RBUS support"
echo "  --disable-telemetry     Disable telemetry support"
echo "  --disable-tests         Don't build tests"
echo ""
