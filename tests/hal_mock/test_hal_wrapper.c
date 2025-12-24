/**
 * @file test_hal_wrapper.c
 * @brief Test HAL wrapper with caching using HAL mock
 */

#include "../../src/core/hal_wrapper/eponMgr_hal_wrapper.h"
#include "../hal_mock/epon_hal_mock.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST_START(name) \
    do { \
        test_count++; \
        printf("\nTest %d: %s\n", test_count, name); \
    } while(0)

#define TEST_PASS() \
    do { \
        pass_count++; \
        printf("  ✓ PASS\n"); \
    } while(0)

#define TEST_FAIL(msg) \
    do { \
        printf("  ✗ FAIL: %s\n", msg); \
        return 1; \
    } while(0)

#define ASSERT_EQ(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            printf("  ✗ FAIL: %s (expected %d, got %d)\n", msg, (int)(expected), (int)(actual)); \
            return 1; \
        } \
    } while(0)

void status_callback(epon_onu_status_t status) {
    printf("  [Callback] Status changed to: %d\n", status);
    // Note: Cannot access wrapper directly since callback doesn't have user_data
    // Status will be updated via the wrapper's callback handling
}

int main(void) {
    printf("=== HAL Wrapper with Caching Test ===\n");
    printf("Testing HAL wrapper against HAL mock\n");
    
    eponMgr_hal_wrapper_t wrapper;
    epon_hal_config_t config;
    
    /* Test 1: Initialize wrapper */
    TEST_START("Initialize HAL wrapper");
    memset(&config, 0, sizeof(config));
    config.struct_size = sizeof(config);
    config.status_callback = status_callback;
    
    int ret = eponMgr_hal_wrapper_init(&wrapper, &config, 2); // 2 second TTL for testing
    ASSERT_EQ(ret, 0, "Wrapper init should succeed");
    TEST_PASS();
    
    /* Test 2: Initialize HAL */
    TEST_START("Initialize HAL through wrapper");
    ret = eponMgr_hal_wrapper_hal_init(&wrapper);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "HAL init should succeed");
    TEST_PASS();
    
    /* Test 3: Get link stats (cache miss) */
    TEST_START("Get link stats (first call - cache miss)");
    epon_hal_link_stats_t stats1;
    memset(&stats1, 0, sizeof(stats1));
    stats1.struct_size = sizeof(stats1);
    
    ret = eponMgr_hal_wrapper_get_link_stats(&wrapper, &stats1);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get link stats should succeed");
    printf("  Stats: TX=%lu, RX=%lu\n", stats1.bytes_sent, stats1.bytes_received);
    TEST_PASS();
    
    /* Test 4: Get link stats (cache hit) */
    TEST_START("Get link stats (second call - cache hit)");
    epon_hal_link_stats_t stats2;
    memset(&stats2, 0, sizeof(stats2));
    stats2.struct_size = sizeof(stats2);
    
    ret = eponMgr_hal_wrapper_get_link_stats(&wrapper, &stats2);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get link stats should succeed");
    
    // Verify cached data matches
    if (stats1.bytes_sent != stats2.bytes_sent || stats1.bytes_received != stats2.bytes_received) {
        TEST_FAIL("Cached stats don't match");
    }
    printf("  Cached stats match original\n");
    TEST_PASS();
    
    /* Test 5: Cache expiration */
    TEST_START("Cache expiration after TTL");
    printf("  Sleeping 3 seconds for cache TTL to expire...\n");
    sleep(3);
    
    epon_hal_link_stats_t stats3;
    memset(&stats3, 0, sizeof(stats3));
    stats3.struct_size = sizeof(stats3);
    
    ret = eponMgr_hal_wrapper_get_link_stats(&wrapper, &stats3);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get link stats should succeed");
    printf("  Got stats after TTL expiry\n");
    TEST_PASS();
    
    /* Test 6: Get OLT info (validity flag cache) */
    TEST_START("Get OLT info (cache miss)");
    epon_olt_info_t olt_info1;
    memset(&olt_info1, 0, sizeof(olt_info1));
    olt_info1.struct_size = sizeof(olt_info1);
    
    ret = eponMgr_hal_wrapper_get_olt_info(&wrapper, &olt_info1);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get OLT info should succeed");
    printf("  OLT Vendor OUI: %02X:%02X:%02X\n", 
           olt_info1.vendor_oui[0], olt_info1.vendor_oui[1], olt_info1.vendor_oui[2]);
    TEST_PASS();
    
    /* Test 7: Get OLT info (cache hit - no TTL) */
    TEST_START("Get OLT info (cache hit - validity flag)");
    epon_olt_info_t olt_info2;
    memset(&olt_info2, 0, sizeof(olt_info2));
    olt_info2.struct_size = sizeof(olt_info2);
    
    ret = eponMgr_hal_wrapper_get_olt_info(&wrapper, &olt_info2);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get OLT info should succeed");
    TEST_PASS();
    
    /* Test 8: Cache invalidation */
    TEST_START("Cache invalidation");
    eponMgr_hal_wrapper_invalidate_cache(&wrapper);
    
    epon_olt_info_t olt_info3;
    memset(&olt_info3, 0, sizeof(olt_info3));
    olt_info3.struct_size = sizeof(olt_info3);
    
    ret = eponMgr_hal_wrapper_get_olt_info(&wrapper, &olt_info3);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get OLT info should succeed");
    TEST_PASS();
    
    /* Test 9: Get LLID list */
    TEST_START("Get LLID list (updates internal data structure)");
    epon_llid_list_t llid_list;
    memset(&llid_list, 0, sizeof(llid_list));
    
    ret = eponMgr_hal_wrapper_get_llid_info(&wrapper, &llid_list);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get LLID info should succeed");
    
    printf("  LLID count: %u\n", llid_list.llid_count);
    
    // Check internal data structure was updated
    uint32_t internal_count = eponMgr_llid_list_count(wrapper.llid_list);
    ASSERT_EQ(internal_count, llid_list.llid_count, "Internal LLID list should be updated");
    TEST_PASS();
    
    /* Test 10: Get interface list */
    TEST_START("Get interface list (updates internal data structure)");
    epon_interface_list_t if_list;
    memset(&if_list, 0, sizeof(if_list));
    
    ret = eponMgr_hal_wrapper_get_interface_list(&wrapper, &if_list);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get interface list should succeed");
    
    printf("  Interface count: %u\n", if_list.interface_count);
    
    // Check internal data structure was updated
    uint32_t if_internal_count = eponMgr_interface_list_count(wrapper.interface_list);
    ASSERT_EQ(if_internal_count, if_list.interface_count, "Internal interface list should be updated");
    TEST_PASS();
    
    /* Test 11: Get manufacturer info */
    TEST_START("Get ONU manufacturer info");
    epon_onu_manufacturer_info_t mfr_info;
    memset(&mfr_info, 0, sizeof(mfr_info));
    mfr_info.struct_size = sizeof(mfr_info);
    
    ret = eponMgr_hal_wrapper_get_onu_manufacturer_info(&wrapper, &mfr_info);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get manufacturer info should succeed");
    printf("  Vendor OUI: %02X:%02X:%02X\n", 
           mfr_info.vendor_oui[0], mfr_info.vendor_oui[1], mfr_info.vendor_oui[2]);
    TEST_PASS();
    
    /* Test 12: Get link info */
    TEST_START("Get link info");
    epon_hal_link_info_t link_info;
    memset(&link_info, 0, sizeof(link_info));
    
    ret = eponMgr_hal_wrapper_get_link_info(&wrapper, &link_info);
    ASSERT_EQ(ret, EPON_HAL_SUCCESS, "Get link info should succeed");
    printf("  Link mode: %s\n", link_info.mode);
    printf("  Encryption: %d\n", link_info.encryption);
    TEST_PASS();
    
    /* Test 13: ONU status through data structure */
    TEST_START("Get ONU status from data structure");
    epon_onu_status_t status;
    ret = eponMgr_onu_state_get_status(wrapper.onu_state, &status);
    ASSERT_EQ(ret, 0, "Should get status from ONU state");
    printf("  Current status: %d\n", status);
    TEST_PASS();
    
    /* Test 14: Direct access to data structures */
    TEST_START("Direct access to internal data structures");
    if (wrapper.interface_list == NULL) TEST_FAIL("interface_list should not be NULL");
    if (wrapper.llid_list == NULL) TEST_FAIL("llid_list should not be NULL");
    if (wrapper.cpe_list == NULL) TEST_FAIL("cpe_list should not be NULL");
    if (wrapper.onu_state == NULL) TEST_FAIL("onu_state should not be NULL");
    if (wrapper.stats_cache == NULL) TEST_FAIL("stats_cache should not be NULL");
    printf("  All data structures accessible\n");
    TEST_PASS();
    
    /* Test 15: Cleanup */
    TEST_START("Destroy wrapper");
    eponMgr_hal_wrapper_destroy(&wrapper);
    printf("  Wrapper destroyed successfully\n");
    TEST_PASS();
    
    /* Summary */
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Passed: %d\n", pass_count);
    printf("Failed: %d\n", test_count - pass_count);
    
    if (pass_count == test_count) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed\n");
        return 1;
    }
}
