/**
 * @file eponMgr_wanmanager.c
 * @brief WanManager notification implementation for EPON Manager
 * 
 * This module handles PHY status notifications to WanManager via RBUS.
 * 
 * Logic:
 * - If ANY interface is UP → notify PHY_STATUS_UP
 * - If ALL interfaces are DOWN → notify PHY_STATUS_DOWN
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "../../../include/eponMgr_rbus.h"
#include "eponMgr_logger.h"

#ifdef USE_DUMMY_RBUS
#include "../../../include/rbus/eponMgr_rbus_dummy.h"
#else
#include <rbus/rbus.h>
#endif

/* WanManager TR-181 parameters */
#define WANMANAGER_PHY_STATUS_PARAM "Device.X_RDK_WanManager.Interface.{i}.BaseInterfaceStatus"
#define WANMANAGER_VIRT_IF_COUNT_PARAM "Device.X_RDK_WanManager.Interface.1.VirtualInterfaceNumberOfEntries"
#define WANMANAGER_VIRT_IF_NAME_PARAM "Device.X_RDK_WanManager.Interface.1.VirtualInterface.%d.Name"
#define WANMANAGER_VIRT_IF_ENABLE_PARAM "Device.X_RDK_WanManager.Interface.1.VirtualInterface.%d.Enable"

/* PHY status values */
#define PHY_STATUS_UP "Up"
#define PHY_STATUS_DOWN "Down"

/**
 * @brief Notify WanManager of PHY status change
 * 
 * @param phy_up true if PHY is UP, false if DOWN
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_notify_wanmanager_phy_status(bool phy_up) {
    EPONMGR_LOG_INFO("PHY Status Change: %s → Notifying WanManager", 
             phy_up ? "UP" : "DOWN");
    
    #ifdef USE_DUMMY_RBUS
    /* Dummy implementation */
    printf("[DUMMY_WANMANAGER] ✓ PHY Status Notification: %s\n", 
           phy_up ? PHY_STATUS_UP : PHY_STATUS_DOWN);
    printf("[DUMMY_WANMANAGER]   Parameter: %s\n", WANMANAGER_PHY_STATUS_PARAM);
    printf("[DUMMY_WANMANAGER]   Value: %s\n", phy_up ? PHY_STATUS_UP : PHY_STATUS_DOWN);
    return 0;
    #else
    /* Real RBUS implementation */
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available");
        return -1;
    }
    
    /* TODO: Phase 10 - Implement real RBUS set call */
    rbusError_t rc = rbus_set(handle, WANMANAGER_PHY_STATUS_PARAM, 
                              NULL /* value */, NULL /* options */);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to set WanManager PHY status: %d", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("Successfully notified WanManager: PHY %s", 
             phy_up ? "UP" : "DOWN");
    return 0;
    #endif
}

/**
 * @brief Update virtual interface table entry
 * 
 * This function searches the WanManager virtual interface table for the specified
 * interface name and updates its Enable status.
 * 
 * @param interface_name Interface name (e.g., "veip0", "erouter0")
 * @param is_up true if interface is UP, false if DOWN
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_update_virtual_interface(const char* interface_name, bool is_up) {
    if (!interface_name) {
        EPONMGR_LOG_ERROR("Invalid interface name");
        return -1;
    }
    
    EPONMGR_LOG_DEBUG("Updating virtual interface: %s = %s", 
              interface_name, is_up ? "UP" : "DOWN");
    
    #ifdef USE_DUMMY_RBUS
    /* Dummy implementation */
    printf("[DUMMY_WANMANAGER] ✓ Virtual Interface Update:\n");
    printf("[DUMMY_WANMANAGER]   Step 1: Query %s\n", WANMANAGER_VIRT_IF_COUNT_PARAM);
    printf("[DUMMY_WANMANAGER]   Step 2: Iterate through VirtualInterface.{i}.Name\n");
    printf("[DUMMY_WANMANAGER]   Step 3: If '%s' found, update Enable to %s\n", 
           interface_name, is_up ? "true" : "false");
    printf("[DUMMY_WANMANAGER]   Step 4: If not found, table add needed (TODO)\n");
    return 0;
    #else
    /* Real RBUS implementation */
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available");
        return -1;
    }
    
    /* Step 1: Get VirtualInterfaceNumberOfEntries */
    /* TODO: Phase 10 - Implement rbus_get() to read count */
    int num_entries = 0; /* Placeholder */
    
    /* Step 2: Iterate through all VirtualInterface entries */
    bool found = false;
    for (int i = 1; i <= num_entries; i++) {
        /* TODO: Phase 10 - Query VirtualInterface.{i}.Name */
        /* char param_name[256]; */
        /* snprintf(param_name, sizeof(param_name), WANMANAGER_VIRT_IF_NAME_PARAM, i); */
        /* rbusValue_t value; */
        /* rbusError_t rc = rbus_get(handle, param_name, &value); */
        /* const char* name = rbusValue_GetString(value, NULL); */
        
        /* if (strcmp(name, interface_name) == 0) { */
        /*     found = true; */
        /*     // Update Enable parameter */
        /*     snprintf(param_name, sizeof(param_name), WANMANAGER_VIRT_IF_ENABLE_PARAM, i); */
        /*     rbusValue_t new_value; */
        /*     rbusValue_Init(&new_value); */
        /*     rbusValue_SetBoolean(new_value, is_up); */
        /*     rc = rbus_set(handle, param_name, new_value, NULL); */
        /*     rbusValue_Release(new_value); */
        /*     break; */
        /* } */
        /* rbusValue_Release(value); */
    }
    
    if (!found) {
        /* TODO: Phase 10 - Implement table add logic for new virtual interface */
        EPONMGR_LOG_WARN("Virtual interface '%s' not found in WanManager table. "
                        "Table add logic not yet implemented.", interface_name);
        return -1;
    }
    
    EPONMGR_LOG_INFO("Updated virtual interface %s: Enable=%s", 
                    interface_name, is_up ? "true" : "false");
    return 0;
    #endif
}


