/**
 * @file test_rbus_basic.c
 * @brief Basic RBUS integration test
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../include/eponMgr_rbus.h"

int main(void) {
    printf("=== EPON Manager RBUS Integration Test ===\n\n");
    
    /* Test 1: Initialize RBUS */
    printf("Test 1: RBUS Initialization\n");
    if (eponMgr_rbus_init("EponManagerTest") == 0) {
        printf("✓ PASSED: RBUS initialized\n\n");
    } else {
        printf("✗ FAILED: RBUS initialization failed\n");
        return 1;
    }
    
    /* Test 2: Get RBUS handle */
    printf("Test 2: Get RBUS Handle\n");
    eponMgr_rbus_handle_t handle = eponMgr_rbus_get_handle();
    if (handle != NULL) {
        printf("✓ PASSED: Got valid RBUS handle\n\n");
    } else {
        printf("✗ FAILED: RBUS handle is NULL\n");
        return 1;
    }
    
    /* Test 3: WanManager PHY status notification */
    printf("Test 3: WanManager PHY Status Notification\n");
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
    
    /* Test 4: Virtual interface update */
    printf("Test 4: Virtual Interface Update\n");
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
    
    /* Test 5: Cleanup */
    printf("Test 5: RBUS Cleanup\n");
    eponMgr_rbus_cleanup();
    printf("✓ PASSED: RBUS cleaned up\n\n");
    
    printf("=== All Tests Passed ===\n");
    return 0;
}
