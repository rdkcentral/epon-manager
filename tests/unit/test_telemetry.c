/**
 * @file test_telemetry.c
 * @brief Unit tests for EPON Manager Telemetry Library (Dummy/Stub Version)
 *
 * Tests all telemetry API functions in dummy mode to verify proper
 * tracing and logging of telemetry calls.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "eponMgr_telemetry.h"
#include "eponMgr_logger.h"

/* Test counter */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ %s\n", message); \
            tests_passed++; \
        } else { \
            printf("  ✗ %s\n", message); \
            tests_failed++; \
        } \
    } while (0)

/**
 * Test 1: Initialize and cleanup telemetry
 */
void test_telemetry_init_cleanup(void) {
    printf("\n[TEST 1] Telemetry Initialization and Cleanup\n");
    
    int ret;
    
    // Test initialization
    ret = eponMgr_telemetry_init("TestComponent");
    TEST_ASSERT(ret == 0, "Telemetry init returns success");
    TEST_ASSERT(eponMgr_telemetry_is_enabled(), "Telemetry is enabled after init");
    
    // Test double initialization (should succeed)
    ret = eponMgr_telemetry_init("TestComponent");
    TEST_ASSERT(ret == 0, "Double init returns success");
    
    // Test cleanup
    ret = eponMgr_telemetry_cleanup();
    TEST_ASSERT(ret == 0, "Telemetry cleanup returns success");
    TEST_ASSERT(!eponMgr_telemetry_is_enabled(), "Telemetry is disabled after cleanup");
    
    // Test double cleanup (should succeed)
    ret = eponMgr_telemetry_cleanup();
    TEST_ASSERT(ret == 0, "Double cleanup returns success");
}

/**
 * Test 2: Event reporting
 */
void test_telemetry_events(void) {
    printf("\n[TEST 2] Telemetry Event Reporting\n");
    
    int ret;
    
    // Initialize first
    eponMgr_telemetry_init("TestComponent");
    
    // Test basic event reporting
    ret = eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_ONU_STATUS_CHANGE,
        "TEST_EVENT",
        "Test data"
    );
    TEST_ASSERT(ret == 0, "Report event with data returns success");
    
    ret = eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_LINK_UP,
        "TEST_LINK_EVENT",
        NULL
    );
    TEST_ASSERT(ret == 0, "Report event without data returns success");
    
    // Test NULL event name (should fail)
    ret = eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_ERROR,
        NULL,
        "data"
    );
    TEST_ASSERT(ret != 0, "Report event with NULL name returns error");
    
    // Test ONU status change event
    ret = eponMgr_telemetry_report_onu_status_change(
        "veip0",
        "Unregistered",
        "Registered"
    );
    TEST_ASSERT(ret == 0, "Report ONU status change returns success");
    
    // Test link up event
    ret = eponMgr_telemetry_report_link_up("veip0");
    TEST_ASSERT(ret == 0, "Report link up returns success");
    
    // Test link down event
    ret = eponMgr_telemetry_report_link_down("veip0");
    TEST_ASSERT(ret == 0, "Report link down returns success");
    
    // Test alarm events with different severities
    ret = eponMgr_telemetry_report_alarm(3, 100, "Critical alarm");
    TEST_ASSERT(ret == 0, "Report critical alarm returns success");
    
    ret = eponMgr_telemetry_report_alarm(2, 101, "Error alarm");
    TEST_ASSERT(ret == 0, "Report error alarm returns success");
    
    ret = eponMgr_telemetry_report_alarm(1, 102, "Warning alarm");
    TEST_ASSERT(ret == 0, "Report warning alarm returns success");
    
    eponMgr_telemetry_cleanup();
}

/**
 * Test 3: Statistics reporting
 */
void test_telemetry_stats(void) {
    printf("\n[TEST 3] Telemetry Statistics Reporting\n");
    
    int ret;
    
    // Initialize first
    eponMgr_telemetry_init("TestComponent");
    
    // Create test statistics array
    eponMgr_telemetry_stat_t stats[5];
    
    for (int i = 0; i < 5; i++) {
        snprintf(stats[i].stat_name, sizeof(stats[i].stat_name), "TEST_STAT_%d", i);
        stats[i].value = 1000 + i;
        stats[i].timestamp = time(NULL);
    }
    
    // Test batch statistics reporting
    ret = eponMgr_telemetry_report_stats(stats, 5);
    TEST_ASSERT(ret == 0, "Report batch stats returns success");
    
    // Test NULL stats (should fail)
    ret = eponMgr_telemetry_report_stats(NULL, 5);
    TEST_ASSERT(ret != 0, "Report NULL stats returns error");
    
    // Test zero count (should fail)
    ret = eponMgr_telemetry_report_stats(stats, 0);
    TEST_ASSERT(ret != 0, "Report zero count stats returns error");
    
    // Test single statistic
    ret = eponMgr_telemetry_report_single_stat("EPON_RxBytes", 123456789);
    TEST_ASSERT(ret == 0, "Report single stat returns success");
    
    // Test single stat with NULL name (should fail)
    ret = eponMgr_telemetry_report_single_stat(NULL, 999);
    TEST_ASSERT(ret != 0, "Report single stat with NULL name returns error");
    
    eponMgr_telemetry_cleanup();
}

/**
 * Test 4: Custom markers
 */
void test_telemetry_markers(void) {
    printf("\n[TEST 4] Telemetry Custom Markers\n");
    
    int ret;
    
    // Initialize first
    eponMgr_telemetry_init("TestComponent");
    
    // Create test marker
    eponMgr_telemetry_marker_t marker;
    strncpy(marker.marker_name, "EPON_TEST_MARKER", sizeof(marker.marker_name) - 1);
    strncpy(marker.value, "Test marker value", sizeof(marker.value) - 1);
    marker.timestamp = time(NULL);
    
    // Test marker sending
    ret = eponMgr_telemetry_send_marker(&marker);
    TEST_ASSERT(ret == 0, "Send marker returns success");
    
    // Test NULL marker (should fail)
    ret = eponMgr_telemetry_send_marker(NULL);
    TEST_ASSERT(ret != 0, "Send NULL marker returns error");
    
    eponMgr_telemetry_cleanup();
}

/**
 * Test 5: Enable/disable functionality
 */
void test_telemetry_enable_disable(void) {
    printf("\n[TEST 5] Telemetry Enable/Disable\n");
    
    int ret;
    
    // Initialize first
    eponMgr_telemetry_init("TestComponent");
    TEST_ASSERT(eponMgr_telemetry_is_enabled(), "Telemetry enabled after init");
    
    // Disable telemetry
    ret = eponMgr_telemetry_set_enabled(false);
    TEST_ASSERT(ret == 0, "Disable telemetry returns success");
    TEST_ASSERT(!eponMgr_telemetry_is_enabled(), "Telemetry is disabled");
    
    // Try to report event while disabled (should succeed but do nothing)
    ret = eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_LINK_UP,
        "TEST_WHILE_DISABLED",
        NULL
    );
    TEST_ASSERT(ret == 0, "Report event while disabled returns success");
    
    // Re-enable telemetry
    ret = eponMgr_telemetry_set_enabled(true);
    TEST_ASSERT(ret == 0, "Enable telemetry returns success");
    TEST_ASSERT(eponMgr_telemetry_is_enabled(), "Telemetry is enabled");
    
    // Try to set enabled without initialization (should fail)
    eponMgr_telemetry_cleanup();
    ret = eponMgr_telemetry_set_enabled(true);
    TEST_ASSERT(ret != 0, "Set enabled without init returns error");
}

/**
 * Test 6: Error conditions
 */
void test_telemetry_error_conditions(void) {
    printf("\n[TEST 6] Telemetry Error Conditions\n");
    
    int ret;
    
    // Try to use telemetry without initialization
    ret = eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_ERROR,
        "TEST_NO_INIT",
        NULL
    );
    TEST_ASSERT(ret != 0, "Report event without init returns error");
    
    ret = eponMgr_telemetry_report_single_stat("TEST_STAT", 100);
    TEST_ASSERT(ret != 0, "Report stat without init returns error");
    
    // Test NULL parameters
    ret = eponMgr_telemetry_init(NULL);
    TEST_ASSERT(ret != 0, "Init with NULL component name returns error");
    
    // Initialize properly for remaining tests
    eponMgr_telemetry_init("TestComponent");
    
    ret = eponMgr_telemetry_report_onu_status_change(NULL, "old", "new");
    TEST_ASSERT(ret != 0, "ONU status change with NULL interface returns error");
    
    ret = eponMgr_telemetry_report_link_up(NULL);
    TEST_ASSERT(ret != 0, "Link up with NULL interface returns error");
    
    ret = eponMgr_telemetry_report_link_down(NULL);
    TEST_ASSERT(ret != 0, "Link down with NULL interface returns error");
    
    eponMgr_telemetry_cleanup();
}

/**
 * Test 7: Multiple events stress test
 */
void test_telemetry_stress(void) {
    printf("\n[TEST 7] Telemetry Stress Test (100 events)\n");
    
    int ret;
    int failures = 0;
    
    eponMgr_telemetry_init("TestComponent");
    
    // Report 100 events rapidly
    for (int i = 0; i < 100; i++) {
        char event_name[64];
        char event_data[128];
        
        snprintf(event_name, sizeof(event_name), "STRESS_EVENT_%d", i);
        snprintf(event_data, sizeof(event_data), "Stress test event number %d", i);
        
        ret = eponMgr_telemetry_report_event(
            EPON_TELEM_EVENT_ERROR,
            event_name,
            event_data
        );
        
        if (ret != 0) {
            failures++;
        }
    }
    
    TEST_ASSERT(failures == 0, "All 100 stress test events succeeded");
    
    eponMgr_telemetry_cleanup();
}

/**
 * Main test runner
 */
int main(void) {
    printf("========================================\n");
    printf("EPON Manager Telemetry Library Tests\n");
    printf("(Dummy/Stub Implementation)\n");
    printf("========================================\n");
    
    // Initialize logger (required for telemetry)
    eponMgr_logger_init();
    
    // Run all tests
    test_telemetry_init_cleanup();
    test_telemetry_events();
    test_telemetry_stats();
    test_telemetry_markers();
    test_telemetry_enable_disable();
    test_telemetry_error_conditions();
    test_telemetry_stress();
    
    // Print summary
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Total Tests:  %d\n", tests_passed + tests_failed);
    printf("========================================\n");
    
    // Cleanup
    eponMgr_logger_close();
    
    if (tests_failed > 0) {
        printf("\n❌ Some tests FAILED\n");
        return 1;
    } else {
        printf("\n✅ All tests PASSED\n");
        return 0;
    }
}
