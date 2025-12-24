# EPON Manager Build and Test Guide

## Overview

The `build_and_test.sh` script provides a comprehensive build and testing solution for the EPON Manager project. It automates the compilation of all components and execution of test suites.

## Quick Start

```bash
# Build and run all tests
./scripts/build_and_test.sh

# Clean build and run all tests
./scripts/build_and_test.sh --clean

# Build only, skip tests
./scripts/build_and_test.sh --no-tests

# Run only unit tests
./scripts/build_and_test.sh --unit-only

# Run with verbose output
./scripts/build_and_test.sh --verbose
```

## Command-Line Options

| Option | Description |
|--------|-------------|
| `--clean` | Clean all build artifacts before building |
| `--unit-only` | Build and run only unit tests |
| `--integration-only` | Build and run only integration tests |
| `--no-tests` | Build only, skip running tests |
| `--verbose` | Show detailed build output (useful for debugging) |
| `--help` | Show help message with usage information |

## What Gets Built

### 1. Core Libraries

The script builds the following libraries in order:

- **Logger** (`libeponMgr_logger.a`) - Logging subsystem
- **Config** (`libeponMgr_config.a`) - Configuration management
- **Data Structures** (`libeponMgr_datastructures.a`) - Core data structures (cache, queue, lists)
- **HAL Wrapper** (`libeponMgr_hal_wrapper.a`) - HAL abstraction layer
- **HAL Mock** (`libepon_hal_mock.so`) - Mock HAL for testing
- **Controller** (`libeponMgr_controller.a`) - Main controller logic

### 2. Main Application

- **EPON Manager** (`src/core/epon_manager`) - Main executable

### 3. Unit Tests

Built from `tests/unit/`:

- `test_logger` - Logger functionality tests
- `test_config` - Configuration system tests
- `test_cache` - ONU status cache tests
- `test_queue` - Event queue tests
- `test_datastructures` - All data structure tests

### 4. Integration Tests

Built from `tests/integration/`:

- `test_event_processing` - Event processing flow tests

## Test Execution

### Unit Tests

The script runs each unit test executable sequentially. Each test reports its own pass/fail status.

**Test Coverage:**
- Logger initialization and log levels
- Configuration file parsing
- Cache operations (add, update, get, invalidate)
- Queue operations (push, pop, full/empty conditions)
- Data structure integrity

### Integration Tests

Integration tests verify component interactions:

- **Event Processing Test**: Simulates HAL callbacks and verifies event flow through the queue to the controller

## Output Format

The script provides color-coded output:

- **Green (✓)**: Success indicators
- **Red (✗)**: Error indicators  
- **Yellow (⚠)**: Warning indicators
- **Blue (ℹ)**: Information messages

### Example Output

```
================================
EPON Manager Build and Test
================================

================================
Building Libraries
================================
ℹ Building logger...
✓ Logger built
ℹ Building config...
✓ Config built
...

================================
Running Unit Tests
================================
ℹ Running test_logger...
✓ test_logger passed
...

================================
Test Summary
================================
Total tests run: 6
Passed: 6
Failed: 0

✓ ALL TESTS PASSED!
```

## Exit Codes

- `0` - All tests passed or build completed successfully
- `1` - One or more tests failed or build error occurred

## Usage Examples

### Typical Development Workflow

```bash
# 1. Make code changes
vim src/core/controller/eponMgr_controller.c

# 2. Quick unit test check
./scripts/build_and_test.sh --unit-only

# 3. Full test with clean build
./scripts/build_and_test.sh --clean

# 4. Run with verbose output to debug issues
./scripts/build_and_test.sh --verbose
```

### CI/CD Integration

```bash
# Clean build and test for CI
./scripts/build_and_test.sh --clean
exit_code=$?

if [ $exit_code -eq 0 ]; then
    echo "Build and tests passed"
else
    echo "Build or tests failed"
    exit 1
fi
```

### Pre-Commit Testing

```bash
# Quick unit test validation before commit
./scripts/build_and_test.sh --unit-only && git commit -m "Your commit message"
```

## Troubleshooting

### Build Failures

If builds fail:

1. Run with `--verbose` to see detailed compiler output
2. Check that all dependencies are available
3. Ensure file permissions are correct
4. Try `--clean` to remove stale artifacts

```bash
./scripts/build_and_test.sh --clean --verbose
```

### Test Failures

If tests fail:

1. Check test logs in `tests/unit/logs/` (for unit tests)
2. Run tests individually for detailed output:
   ```bash
   cd tests/unit
   ./test_logger
   ./test_cache
   ```
3. Check for memory issues with valgrind:
   ```bash
   cd tests/unit
   valgrind --leak-check=full ./test_cache
   ```

### Common Issues

**Issue**: "make: *** No rule to make target"
- **Solution**: Run with `--clean` to rebuild from scratch

**Issue**: Shared library not found at runtime
- **Solution**: Check LD_LIBRARY_PATH or rpath settings in Makefiles

**Issue**: Permission denied
- **Solution**: Ensure script is executable: `chmod +x scripts/build_and_test.sh`

## Directory Structure

```
epon-manager/
├── scripts/
│   └── build_and_test.sh      # Main build script
├── src/
│   ├── logger/                # Logger component
│   └── core/
│       ├── config/            # Configuration component
│       ├── data_structures/   # Data structures component
│       ├── hal_wrapper/       # HAL wrapper component
│       ├── controller/        # Controller component
│       └── epon_manager_main.c
├── tests/
│   ├── hal_mock/              # Mock HAL implementation
│   ├── unit/                  # Unit tests
│   │   ├── test_logger.c
│   │   ├── test_config.c
│   │   ├── test_cache.c
│   │   ├── test_queue.c
│   │   └── test_datastructures.c
│   └── integration/           # Integration tests
│       └── test_event_processing.c
└── lib/                       # Built shared libraries
```

## Adding New Tests

### Adding a Unit Test

1. Create test source file in `tests/unit/`:
   ```bash
   touch tests/unit/test_new_feature.c
   ```

2. Add test to `tests/unit/Makefile`:
   ```makefile
   TEST_NEW_FEATURE = test_new_feature
   TESTS = ... $(TEST_NEW_FEATURE)
   
   $(TEST_NEW_FEATURE): test_new_feature.c dependencies...
       $(CC) $(CFLAGS) test_new_feature.c -o $(TEST_NEW_FEATURE) $(LDFLAGS)
   ```

3. Add to build script's test list in `run_unit_tests()` function:
   ```bash
   local test_executables=(... test_new_feature)
   ```

### Adding an Integration Test

1. Create test in `tests/integration/`
2. Add build logic to `build_integration_tests()` function in build script
3. Add execution logic to `run_integration_tests()` function

## Performance Considerations

- **Parallel Builds**: Components are built sequentially to ensure dependencies are satisfied
- **Incremental Builds**: Only changed files are recompiled (unless `--clean` is used)
- **Test Isolation**: Each test runs in its own process for isolation

## Best Practices

1. **Always run tests before committing**: Use `--unit-only` for quick validation
2. **Use clean builds periodically**: Prevents issues from stale artifacts
3. **Check verbose output for warnings**: Compiler warnings can indicate potential issues
4. **Run full test suite before releases**: Use default mode with no flags
5. **Document new tests**: Update this guide when adding new test cases

## Related Documentation
docs/implementation/IMPLEMENTATION_CHECKLIST.md](../docs/implementation/IMPLEMENTATION_CHECKLIST.md) - Project implementation status
- [docs/implementation/PHASE5_EVENT_LISTENER_SUMMARY.md](../docs/implementation/PHASE5_EVENT_LISTENER_SUMMARY.md) - Event processing details
- [README.md](../Summary](PHASE5_EVENT_LISTENER_SUMMARY.md) - Event processing details
- [README](README.md) - Project overview

## Support

For issues or questions:scripts/build_and_test.sh --verbose`
- Review individual test logs in `tests/unit/logs/`
- Ensure all dependencies are installed
- Try clean rebuild: `./scriptses are installed
- Try clean rebuild: `./build_and_test.sh --clean`
