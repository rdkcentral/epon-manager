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
#include <rbus/rbus.h>

/* WanManager TR-181 parameters */
#define WANMANAGER_PHY_STATUS_PARAM "Device.X_RDK_WanManager.Interface.2.BaseInterfaceStatus"
#define WANMANAGER_VIRT_IF_COUNT_PARAM "Device.X_RDK_WanManager.Interface.2.VirtualInterfaceNumberOfEntries"
#define WANMANAGER_VIRT_IF_NAME_PARAM "Device.X_RDK_WanManager.Interface.2.VirtualInterface.%d.Name"
#define WANMANAGER_VIRT_IF_ENABLE_PARAM "Device.X_RDK_WanManager.Interface.2.VirtualInterface.%d.Enable"

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
    EPONMGR_LOG_INFO("PHY Status Change: %s → Notifying WanManager\n", 
             phy_up ? "UP" : "DOWN");
    
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available\n");
        return -1;
    }
    
    /* Create value for PHY status */
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, phy_up ? PHY_STATUS_UP : PHY_STATUS_DOWN);
    
    /* Set the parameter in WanManager */
    rbusError_t rc = rbus_set(handle, WANMANAGER_PHY_STATUS_PARAM, 
                              value, NULL);
    rbusValue_Release(value);
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to set WanManager PHY status: error=%d\n", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("Successfully notified WanManager: PHY %s\n", 
             phy_up ? "UP" : "DOWN");
    return 0;
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
        EPONMGR_LOG_ERROR("Invalid interface name\n");
        return -1;
    }
    
    EPONMGR_LOG_DEBUG("Updating virtual interface: %s = %s\n", 
              interface_name, is_up ? "UP" : "DOWN");
    
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available\n");
        return -1;
    }
    
    /* Step 1: Query VirtualInterfaceNumberOfEntries to get table size */
    rbusValue_t count_value = NULL;
    rbusError_t rc = rbus_get(handle, WANMANAGER_VIRT_IF_COUNT_PARAM, &count_value);
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_WARN("Failed to query virtual interface count: error=%d\n", rc);
        return -1;
    }
    
    int num_entries = (int)rbusValue_GetUInt32(count_value);
    rbusValue_Release(count_value);
    
    EPONMGR_LOG_DEBUG("Found %d virtual interface entries in WanManager\n", num_entries);
    
    /* Step 2: Iterate through all VirtualInterface entries */
    bool found = false;
    for (int i = 1; i <= num_entries; i++) {
        char param_name[256];
        snprintf(param_name, sizeof(param_name), WANMANAGER_VIRT_IF_NAME_PARAM, i);
        
        /* Query VirtualInterface.{i}.Name */
        rbusValue_t name_value = NULL;
        rc = rbus_get(handle, param_name, &name_value);
        
        if (rc != RBUS_ERROR_SUCCESS) {
            EPONMGR_LOG_DEBUG("Failed to query interface %d name: error=%d\n", i, rc);
            continue;
        }
        
        const char* name = rbusValue_GetString(name_value, NULL);
        if (!name) {
            rbusValue_Release(name_value);
            continue;
        }
        
        EPONMGR_LOG_DEBUG("Checking interface [%d]: %s\n", i, name);
        
        /* Check if this is the interface we're looking for */
        if (strcmp(name, interface_name) == 0) {
            found = true;
            rbusValue_Release(name_value);
            
            /* Update Enable parameter */
            snprintf(param_name, sizeof(param_name), WANMANAGER_VIRT_IF_ENABLE_PARAM, i);
            
            rbusValue_t enable_value;
            rbusValue_Init(&enable_value);
            rbusValue_SetBoolean(enable_value, is_up);
            
            rc = rbus_set(handle, param_name, enable_value, NULL);
            rbusValue_Release(enable_value);
            
            if (rc != RBUS_ERROR_SUCCESS) {
                EPONMGR_LOG_ERROR("Failed to set interface %s Enable status: error=%d\n", 
                                 interface_name, rc);
                return -1;
            }
            
            EPONMGR_LOG_INFO("Updated virtual interface %s: Enable=%s\n", 
                           interface_name, is_up ? "true" : "false");
            break;
        }
        
        rbusValue_Release(name_value);
    }
    
    if (!found) {
        EPONMGR_LOG_WARN("Virtual interface '%s' not found in WanManager table.\n"
                        "Cannot update status. Table add logic would be needed here.\n", 
                        interface_name);
        //TODO: Implement table entry addition, this needs support from the WanManager side too
        return -1;
    }
    
    return 0;
}


