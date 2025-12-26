#!/bin/bash

################################################################################
# EPON Manager Build and Test Script
# 
# This script builds the EPON Manager application, HAL mock library,
# unit tests, and integration tests, then runs all test cases.
#
# Usage:
#   ./build_and_test.sh [options]
#
# Options:
#   --clean         Clean all build artifacts before building
#   --unit-only     Build and run only unit tests
#   --integration-only  Build and run only integration tests
#   --no-tests      Build only, skip running tests
#   --verbose       Show detailed build output
#   --help          Show this help message
################################################################################

set -e  # Exit on error

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default options
CLEAN=false
UNIT_ONLY=false
INTEGRATION_ONLY=false
NO_TESTS=false
VERBOSE=false

# Directories - script is in scripts/ directory, go up one level to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SRC_DIR="$PROJECT_ROOT/src"
TESTS_DIR="$PROJECT_ROOT/tests"
LIB_DIR="$PROJECT_ROOT/lib"

# Counters
TESTS_PASSED=0
TESTS_FAILED=0
TESTS_TOTAL=0

################################################################################
# Helper Functions
################################################################################

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ $1${NC}"
}

show_help() {
    head -n 20 "$0" | grep "^#" | sed 's/^# //' | sed 's/^#//'
    exit 0
}

################################################################################
# Parse Command Line Arguments
################################################################################

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                CLEAN=true
                shift
                ;;
            --unit-only)
                UNIT_ONLY=true
                shift
                ;;
            --integration-only)
                INTEGRATION_ONLY=true
                shift
                ;;
            --no-tests)
                NO_TESTS=true
                shift
                ;;
            --verbose)
                VERBOSE=true
                shift
                ;;
            --help)
                show_help
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
}

################################################################################
# Build Functions
################################################################################

clean_all() {
    print_header "Cleaning Build Artifacts"
    
    cd "$PROJECT_ROOT"
    
    # Clean core application
    if [ -f "$SRC_DIR/core/Makefile" ]; then
        print_info "Cleaning core application..."
        make -C "$SRC_DIR/core" clean 2>/dev/null || true
    fi
    
    # Clean HAL mock
    if [ -f "$TESTS_DIR/hal_mock/Makefile" ]; then
        print_info "Cleaning HAL mock..."
        make -C "$TESTS_DIR/hal_mock" clean 2>/dev/null || true
    fi
    
    # Clean unit tests
    if [ -f "$TESTS_DIR/unit/Makefile" ]; then
        print_info "Cleaning unit tests..."
        make -C "$TESTS_DIR/unit" clean 2>/dev/null || true
    fi
    
    # Clean integration tests
    rm -f "$TESTS_DIR/integration/test_event_processing"
    rm -f "$TESTS_DIR/test_controller_with_mock"
    
    # Clean libraries
    rm -rf "$LIB_DIR"
    
    print_success "Clean complete"
    echo ""
}

build_libraries() {
    print_header "Building Libraries"
    
    cd "$PROJECT_ROOT"
    
    # Build logger
    if [ -f "$SRC_DIR/logger/Makefile" ]; then
        print_info "Building logger..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/logger"
        else
            make -C "$SRC_DIR/logger" > /dev/null 2>&1
        fi
        print_success "Logger built"
    fi
    
    # Build config
    if [ -f "$SRC_DIR/core/config/Makefile" ]; then
        print_info "Building config..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/core/config"
        else
            make -C "$SRC_DIR/core/config" > /dev/null 2>&1
        fi
        print_success "Config built"
    fi
    
    # Build data structures
    if [ -f "$SRC_DIR/core/data_structures/Makefile" ]; then
        print_info "Building data structures..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/core/data_structures"
        else
            make -C "$SRC_DIR/core/data_structures" > /dev/null 2>&1
        fi
        print_success "Data structures built"
    fi
    
    # Build HAL wrapper
    if [ -f "$SRC_DIR/core/hal_wrapper/Makefile" ]; then
        print_info "Building HAL wrapper..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/core/hal_wrapper"
        else
            make -C "$SRC_DIR/core/hal_wrapper" > /dev/null 2>&1
        fi
        print_success "HAL wrapper built"
    fi
    
    # Build HAL mock
    if [ -f "$TESTS_DIR/hal_mock/Makefile" ]; then
        print_info "Building HAL mock..."
        if [ "$VERBOSE" = true ]; then
            make -C "$TESTS_DIR/hal_mock"
        else
            make -C "$TESTS_DIR/hal_mock" > /dev/null 2>&1
        fi
        print_success "HAL mock built"
    fi
    
    # Build controller
    if [ -f "$SRC_DIR/core/controller/Makefile" ]; then
        print_info "Building controller..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/core/controller"
        else
            make -C "$SRC_DIR/core/controller" > /dev/null 2>&1
        fi
        print_success "Controller built"
    fi
    
    # Build RBUS integration
    if [ -f "$SRC_DIR/rbus/Makefile" ]; then
        print_info "Building RBUS integration..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/rbus"
        else
            make -C "$SRC_DIR/rbus" > /dev/null 2>&1
        fi
        print_success "RBUS integration built"
    fi
    
    echo ""
}

build_main_application() {
    print_header "Building Main Application"
    
    cd "$PROJECT_ROOT"
    
    if [ -f "$SRC_DIR/core/Makefile" ]; then
        print_info "Building EPON Manager..."
        if [ "$VERBOSE" = true ]; then
            make -C "$SRC_DIR/core"
        else
            make -C "$SRC_DIR/core" > /dev/null 2>&1
        fi
        print_success "EPON Manager built: $SRC_DIR/core/epon_manager"
    else
        print_error "Core Makefile not found"
        exit 1
    fi
    
    echo ""
}

build_unit_tests() {
    print_header "Building Unit Tests"
    
    cd "$PROJECT_ROOT"
    
    if [ -f "$TESTS_DIR/unit/Makefile" ]; then
        print_info "Building unit tests..."
        if [ "$VERBOSE" = true ]; then
            make -C "$TESTS_DIR/unit"
        else
            make -C "$TESTS_DIR/unit" > /dev/null 2>&1
        fi
        print_success "Unit tests built"
    else
        print_warning "Unit tests Makefile not found"
    fi
    
    echo ""
}

build_integration_tests() {
    print_header "Building Integration Tests"
    
    cd "$PROJECT_ROOT"
    
    # Build integration test executable
    if [ -f "$TESTS_DIR/integration/test_event_processing.c" ]; then
        print_info "Building test_event_processing..."
        gcc -Wall -Wextra -O2 \
            -I"$PROJECT_ROOT/include" \
            -I"$TESTS_DIR/hal_mock" \
            "$TESTS_DIR/integration/test_event_processing.c" \
            -L"$LIB_DIR" \
            -lepon_hal_mock \
            -Wl,-rpath,"$LIB_DIR" \
            -o "$TESTS_DIR/integration/test_event_processing"
        
        if [ $? -eq 0 ]; then
            print_success "Integration test built: $TESTS_DIR/integration/test_event_processing"
        else
            print_error "Failed to build integration test"
        fi
    fi
    
    echo ""
}

################################################################################
# Test Execution Functions
################################################################################

run_unit_tests() {
    print_header "Running Unit Tests"
    
    cd "$TESTS_DIR/unit"
    
    # Get list of test executables
    local test_executables=(test_logger test_config test_cache test_queue test_datastructures test_rbus_basic)
    
    for test in "${test_executables[@]}"; do
        if [ -f "$test" ]; then
            TESTS_TOTAL=$((TESTS_TOTAL + 1))
            print_info "Running $test..."
            
            if [ "$VERBOSE" = true ]; then
                ./"$test"
            else
                ./"$test" > /dev/null 2>&1
            fi
            
            if [ $? -eq 0 ]; then
                print_success "$test passed"
                TESTS_PASSED=$((TESTS_PASSED + 1))
            else
                print_error "$test failed"
                TESTS_FAILED=$((TESTS_FAILED + 1))
            fi
        else
            print_warning "$test not found (skipped)"
        fi
    done
    
    echo ""
}

run_integration_tests() {
    print_header "Running Integration Tests"
    
    cd "$PROJECT_ROOT"
    
    # Test 1: Event Processing Test
    if [ -f "$TESTS_DIR/integration/test_event_processing" ]; then
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        print_info "Running event processing test..."
        
        if [ "$VERBOSE" = true ]; then
            "$TESTS_DIR/integration/test_event_processing"
        else
            "$TESTS_DIR/integration/test_event_processing" > /dev/null 2>&1
        fi
        
        if [ $? -eq 0 ]; then
            print_success "Event processing test passed"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            print_error "Event processing test failed"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        print_warning "Event processing test not found (skipped)"
    fi
    
    # Test 2: Controller with Mock HAL
    if [ -f "$TESTS_DIR/test_controller_with_mock" ]; then
        TESTS_TOTAL=$((TESTS_TOTAL + 1))
        print_info "Running controller with mock HAL test (3s timeout)..."
        
        if [ "$VERBOSE" = true ]; then
            timeout 3 "$TESTS_DIR/test_controller_with_mock"
        else
            timeout 3 "$TESTS_DIR/test_controller_with_mock" > /dev/null 2>&1
        fi
        
        local exit_code=$?
        # Exit code 124 means timeout (expected)
        if [ $exit_code -eq 124 ] || [ $exit_code -eq 0 ]; then
            print_success "Controller test passed"
            TESTS_PASSED=$((TESTS_PASSED + 1))
        else
            print_error "Controller test failed (exit code: $exit_code)"
            TESTS_FAILED=$((TESTS_FAILED + 1))
        fi
    else
        print_warning "Controller test not found (skipped)"
    fi
    
    echo ""
}

print_test_summary() {
    print_header "Test Summary"
    
    echo "Total tests run: $TESTS_TOTAL"
    echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
    
    if [ $TESTS_FAILED -gt 0 ]; then
        echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    else
        echo -e "Failed: $TESTS_FAILED"
    fi
    
    echo ""
    
    if [ $TESTS_FAILED -eq 0 ] && [ $TESTS_TOTAL -gt 0 ]; then
        print_success "ALL TESTS PASSED!"
        return 0
    elif [ $TESTS_TOTAL -eq 0 ]; then
        print_warning "No tests were run"
        return 0
    else
        print_error "SOME TESTS FAILED!"
        return 1
    fi
}

################################################################################
# Main Execution
################################################################################

main() {
    parse_arguments "$@"
    
    echo ""
    print_header "EPON Manager Build and Test"
    echo ""
    
    # Clean if requested
    if [ "$CLEAN" = true ]; then
        clean_all
    fi
    
    # Build phase
    if [ "$INTEGRATION_ONLY" = false ]; then
        build_libraries
        build_main_application
    fi
    
    if [ "$INTEGRATION_ONLY" = false ]; then
        build_unit_tests
    fi
    
    if [ "$UNIT_ONLY" = false ]; then
        build_integration_tests
    fi
    
    # Test phase
    if [ "$NO_TESTS" = false ]; then
        if [ "$INTEGRATION_ONLY" = false ]; then
            run_unit_tests
        fi
        
        if [ "$UNIT_ONLY" = false ]; then
            run_integration_tests
        fi
        
        print_test_summary
        exit $?
    else
        print_info "Build complete. Skipping tests (--no-tests flag)"
        exit 0
    fi
}

# Run main function
main "$@"
