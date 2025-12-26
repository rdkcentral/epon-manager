/**
 * @file test_rbus_basic.c
 * @brief Basic RBUS integration test with HAL wrapper context
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/eponMgr_rbus.h"
#include "../../src/core/hal_wrapper/eponMgr_hal_wrapper.h"
#include "../../src/core/data_structures/eponMgr_onu_state.h"
#include "../../src/core/data_structures/eponMgr_llid_list.h"
#include "../../src/core/data_structures/eponMgr_cpe_list.h"
#include "../../src/core/data_structures/eponMgr_cache.h"
#include "../../include/epon_hal.h"

int main(void) {
    printf("=== EPON Manager RBUS Integration Test ===\n\n");
    
    /* Create a test HAL wrapper context */
    eponMgr_hal_wrapper_t hal_wrapper;
    memset(&hal_wrapper, 0, sizeof(hal_wrapper));
    
    /* Initialize HAL wrapper components */
    eponMgr_onu_state_t onu_state;
    eponMgr_onu_state_init(&onu_state, true);  // Enable DPoE support
    hal_wrapper.onu_state = &onu_state;
    
    eponMgr_llid_list_t llid_list;
    eponMgr_llid_list_init(&llid_list, 8);
    hal_wrapper.llid_list = &llid_list;
    
    eponMgr_cpe_list_t cpe_list;
    eponMgr_cpe_list_init(&cpe_list, 32);
    hal_wrapper.cpe_list = &cpe_list;
    
    eponMgr_cache_t stats_cache;
    eponMgr_cache_init(&stats_cache, 30);
    hal_wrapper.stats_cache = &stats_cache;
    
    hal_wrapper.hal_config.dpoe_supported = false;
    hal_wrapper.hal_initialized = false;
    
    printf("✓ HAL wrapper context initialized\n\n");
    
    /* Test 1: Initialize RBUS */
    printf("Test 1: RBUS Initialization\n");
    if (eponMgr_rbus_init("epon.manager.test", &hal_wrapper) == 0) {
        printf("✓ PASSED: RBUS initialized\n\n");
    } else {
        printf("✗ FAILED: RBUS initialization failed\n");
        return 1;
    }
    
    /* Test 2: WanManager PHY status notification */
    printf("Test 2: WanManager PHY Status Notification\n");
    if (eponMgr_rbus_notify_wanmanager_phy_status(true) == 0) {
        printf("✓ PASSED: PHY UP notification sent\n");
    } else {
        printf("✗ FAILED: PHY UP notification failed\n");
        return 1;
    }
    
    if (eponMgr_rbus_notify_wanmanager_phy_status(false) == 0) {
        printf("✓ PASSED: PHY DOWN notification sent\n\n");
    } else {
        printf("✗ FAILED: PHY DOWN notification failed\n");
        return 1;
    }
    
    /* Test 3: Virtual interface update */
    printf("Test 3: Virtual Interface Update\n");
    if (eponMgr_rbus_update_virtual_interface("veip0", true) == 0) {
        printf("✓ PASSED: Interface veip0 UP\n");
    } else {
        printf("✗ FAILED: Interface update failed\n");
        return 1;
    }
    
    if (eponMgr_rbus_update_virtual_interface("veip0", false) == 0) {
        printf("✓ PASSED: Interface veip0 DOWN\n\n");
    } else {
        printf("✗ FAILED: Interface update failed\n");
        return 1;
    }
    
    /* Test 4: Cleanup */
    printf("Test 4: RBUS Cleanup\n");
    eponMgr_rbus_cleanup();
    printf("✓ PASSED: RBUS cleaned up\n\n");
    
    /* Cleanup HAL wrapper components */
    eponMgr_cache_destroy(&stats_cache);
    eponMgr_cpe_list_destroy(&cpe_list);
    eponMgr_llid_list_destroy(&llid_list);
    eponMgr_onu_state_destroy(&onu_state);
    
    printf("=== All Tests Passed ===\n");
    return 0;
}
