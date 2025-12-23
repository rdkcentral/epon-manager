/**
 * @file test_cache.c
 * @brief Test cache with two strategies: TTL for stats, validity flag for info
 */

#include "../../src/core/data_structures/eponMgr_cache.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

int main(void) {
    printf("=== EPON Manager Cache Test ===\n\n");
    
    /* Test 1: Initialize cache */
    printf("Test 1: Initialize cache with 5 second TTL (for stats only)\n");
    eponMgr_cache_t cache;
    eponMgr_cache_init(&cache, 5);
    
    if (cache.ttl_seconds == 5) {
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL\n\n");
        return 1;
    }
    
    /* Test 2: Cache miss on empty cache */
    printf("Test 2: Get from empty cache (should be cache miss)\n");
    epon_hal_link_stats_t link_stats = {0};
    link_stats.struct_size = sizeof(link_stats);
    
    if (!eponMgr_cache_get_link_stats(&cache, &link_stats)) {
        printf("  ✓ PASS - cache miss as expected\n\n");
    } else {
        printf("  ✗ FAIL - should be cache miss\n\n");
        return 1;
    }
    
    /* Test 3: Store and retrieve statistics (cache hit) */
    printf("Test 3: Store stats and retrieve immediately (should be cache hit)\n");
    link_stats.packets_sent = 1000;
    link_stats.packets_received = 2000;
    eponMgr_cache_set_link_stats(&cache, &link_stats);
    
    epon_hal_link_stats_t retrieved = {0};
    retrieved.struct_size = sizeof(retrieved);
    if (eponMgr_cache_get_link_stats(&cache, &retrieved)) {
        if (retrieved.packets_sent == 1000 && retrieved.packets_received == 2000) {
            printf("  ✓ PASS - cache hit with correct data\n\n");
        } else {
            printf("  ✗ FAIL - data mismatch\n\n");
            return 1;
        }
    } else {
        printf("  ✗ FAIL - should be cache hit\n\n");
        return 1;
    }
    
    /* Test 4: Wait for statistics TTL expiration */
    printf("Test 4: Wait 6 seconds for stats cache to expire (TTL=5s)...\n");
    printf("  (This will take a moment)\n");
    sleep(6);
    
    if (!eponMgr_cache_get_link_stats(&cache, &retrieved)) {
        printf("  ✓ PASS - stats cache miss after expiration\n\n");
    } else {
        printf("  ✗ FAIL - should be cache miss after TTL\n\n");
        return 1;
    }
    
    /* Test 5: Info cache (no TTL expiration) */
    printf("Test 5: Info cache does NOT expire with TTL\n");
    epon_onu_manufacturer_info_t mfg_info = {0};
    mfg_info.struct_size = sizeof(mfg_info);
    strncpy(mfg_info.manufacturer, "Test Vendor", EPON_HAL_MANUFACTURER_LEN - 1);
    eponMgr_cache_set_manufacturer_info(&cache, &mfg_info);
    
    printf("  Stored manufacturer info, waiting 6 seconds...\n");
    sleep(6);
    
    epon_onu_manufacturer_info_t retrieved_mfg = {0};
    retrieved_mfg.struct_size = sizeof(retrieved_mfg);
    if (eponMgr_cache_get_manufacturer_info(&cache, &retrieved_mfg) &&
        strcmp(retrieved_mfg.manufacturer, "Test Vendor") == 0) {
        printf("  ✓ PASS - info cache still valid after 6s (no TTL)\n\n");
    } else {
        printf("  ✗ FAIL - info cache should remain valid\n\n");
        return 1;
    }
    
    /* Test 6: Manual cache invalidation for stats */
    printf("Test 6: Manual invalidation of stats cache\n");
    eponMgr_cache_set_link_stats(&cache, &link_stats);
    
    /* Should be cache hit before invalidation */
    if (eponMgr_cache_get_link_stats(&cache, &retrieved)) {
        printf("  Before invalidation: cache hit ✓\n");
    } else {
        printf("  ✗ FAIL - should be cache hit before invalidation\n\n");
        return 1;
    }
    
    /* Invalidate */
    eponMgr_cache_invalidate(&cache, "link_stats");
    
    /* Should be cache miss after invalidation */
    if (!eponMgr_cache_get_link_stats(&cache, &retrieved)) {
        printf("  After invalidation: cache miss ✓\n");
        printf("  ✓ PASS\n\n");
    } else {
        printf("  ✗ FAIL - should be cache miss after invalidation\n\n");
        return 1;
    }
    
    /* Test 7: Invalidate all on ONU status change */
    printf("Test 7: Invalidate all cache entries (simulating ONU status change)\n");
    eponMgr_cache_set_link_stats(&cache, &link_stats);
    
    epon_hal_transceiver_stats_t xcvr_stats = {0};
    xcvr_stats.struct_size = sizeof(xcvr_stats);
    xcvr_stats.temperature = 45.5f;
    eponMgr_cache_set_transceiver_stats(&cache, &xcvr_stats);
    
    eponMgr_cache_set_manufacturer_info(&cache, &mfg_info);
    
    epon_hal_link_info_t link_info = {0};
    strncpy(link_info.mode, "1G-EPON", EPON_HAL_MODE_LEN - 1);
    eponMgr_cache_set_link_info(&cache, &link_info);
    
    /* Invalidate all (would be called on ONU status change) */
    eponMgr_cache_invalidate_all(&cache);
    
    epon_hal_link_info_t retrieved_link = {0};
    retrieved_mfg.struct_size = sizeof(retrieved_mfg);
    
    if (!eponMgr_cache_get_link_stats(&cache, &retrieved) &&
        !eponMgr_cache_get_transceiver_stats(&cache, &xcvr_stats) &&
        !eponMgr_cache_get_manufacturer_info(&cache, &retrieved_mfg) &&
        !eponMgr_cache_get_link_info(&cache, &retrieved_link)) {
        printf("  ✓ PASS - all cache entries invalidated\n\n");
    } else {
        printf("  ✗ FAIL - not all entries invalidated\n\n");
        return 1;
    }
    
    printf("=== All Cache Tests Passed ===\n");
    printf("Summary:\n");
    printf("  - Statistics use TTL-based expiration\n");
    printf("  - Info data uses validity flag only\n");
    printf("  - All cache invalidated on ONU status change\n");
    return 0;
}
