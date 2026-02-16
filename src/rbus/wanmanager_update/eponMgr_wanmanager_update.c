/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file eponMgr_wanmanager_update.c
 * @brief WanManager status update implementation for EPON Manager
 * 
 * This module handles PHY status and interface updates to WanManager via RBUS.
 * 
 * Logic:
 * - If ANY interface is UP → notify PHY_STATUS_UP
 * - If ALL interfaces are DOWN → notify PHY_STATUS_DOWN
 * - WanManager interface index is discovered dynamically by searching for
 *   the interface with BaseInterface = "Device.Optical.Interface.1"
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "eponMgr_rbus.h"
#include "eponMgr_logger.h"
#include <rbus/rbus.h>

/* WanManager TR-181 base parameters */
#define WANMANAGER_BASE_PATH "Device.X_RDK_WanManager"
#define WANMANAGER_IF_COUNT_PARAM "Device.X_RDK_WanManager.InterfaceNumberOfEntries"
#define WANMANAGER_IF_BASE_IF_PARAM "Device.X_RDK_WanManager.Interface.%d.BaseInterface"
#define EPON_BASE_INTERFACE_PATH "Device.Optical.Interface.1"

/* PHY status values */
#define PHY_STATUS_UP "Up"
#define PHY_STATUS_DOWN "Down"

/* Cached WanManager interface index (0 = not yet discovered) */
static int g_wanmanager_if_index = 0;

/**
 * @brief Discover WanManager interface index for EPON
 * 
 * Queries WanManager InterfaceNumberOfEntries and searches for the interface
 * with BaseInterface = "Device.Optical.Interface.1". The discovered index is
 * cached for subsequent operations.
 * 
 * @return Interface index (1-based) on success, -1 on failure
 * 
 * @note Called automatically on first use by public functions
 * @note Index is cached in g_wanmanager_if_index and reused
 * @note Only queries once - subsequent calls return cached value
 */
static int discover_wanmanager_interface_index(void) {
    /* Return cached value if already discovered */
    if (g_wanmanager_if_index > 0) {
        return g_wanmanager_if_index;
    }
    
    EPONMGR_LOG_INFO("Discovering WanManager interface index for EPON...\n");
    
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available\n");
        return -1;
    }
    
    /* Query InterfaceNumberOfEntries */
    rbusValue_t count_value = NULL;
    rbusError_t rc = rbus_get(handle, WANMANAGER_IF_COUNT_PARAM, &count_value);
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to query WanManager interface count: error=%d\n", rc);
        return -1;
    }
    
    int num_entries = (int)rbusValue_GetUInt32(count_value);
    rbusValue_Release(count_value);
    
    EPONMGR_LOG_DEBUG("Found %d WanManager interface entries\n", num_entries);
    
    /* Search for interface with BaseInterface = "Device.Optical.Interface.1" */
    for (int i = 1; i <= num_entries; i++) {
        char param_name[256];
        snprintf(param_name, sizeof(param_name), WANMANAGER_IF_BASE_IF_PARAM, i);
        
        rbusValue_t base_if_value = NULL;
        rc = rbus_get(handle, param_name, &base_if_value);
        
        if (rc != RBUS_ERROR_SUCCESS) {
            EPONMGR_LOG_DEBUG("Failed to query interface %d BaseInterface: error=%d\n", i, rc);
            continue;
        }
        
        const char* base_if = rbusValue_GetString(base_if_value, NULL);
        if (base_if && strcmp(base_if, EPON_BASE_INTERFACE_PATH) == 0) {
            rbusValue_Release(base_if_value);
            g_wanmanager_if_index = i;
            EPONMGR_LOG_INFO("Discovered EPON WanManager interface index: %d (BaseInterface=%s)\n", 
                           i, EPON_BASE_INTERFACE_PATH);
            return i;
        }
        
        rbusValue_Release(base_if_value);
    }
    
    EPONMGR_LOG_ERROR("Failed to find WanManager interface with BaseInterface='%s'\n", 
                     EPON_BASE_INTERFACE_PATH);
    return -1;
}

/**
 * @brief Notify WanManager of PHY status change
 * 
 * Updates the WanManager BaseInterfaceStatus parameter to indicate overall
 * PHY status. PHY is UP if any interface is UP, DOWN if all interfaces are DOWN.
 * Automatically discovers the correct WanManager interface index on first call.
 * 
 * @param phy_up true if PHY should be reported as UP, false for DOWN
 * @return 0 on success, -1 on failure
 * 
 * @note Uses RBUS rbus_set() to update WanManager parameter
 * @note Parameter: Device.X_RDK_WanManager.Interface.{index}.BaseInterfaceStatus
 * @note Requires RBUS handle from eponMgr_rbus_get_handle()
 */
int eponMgr_rbus_notify_wanmanager_phy_status(bool phy_up) {
    EPONMGR_LOG_INFO("PHY Status Change: %s → Notifying WanManager\n", 
             phy_up ? "UP" : "DOWN");
    
    /* Discover WanManager interface index if not already cached */
    int if_index = discover_wanmanager_interface_index();
    if (if_index < 0) {
        EPONMGR_LOG_ERROR("Failed to discover WanManager interface index\n");
        return -1;
    }
    
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available\n");
        return -1;
    }
    
    /* Build parameter path dynamically */
    char param_path[256];
    snprintf(param_path, sizeof(param_path), 
             "Device.X_RDK_WanManager.Interface.%d.BaseInterfaceStatus", if_index);
    
    /* Create value for PHY status */
    rbusValue_t value;
    rbusValue_Init(&value);
    rbusValue_SetString(value, phy_up ? PHY_STATUS_UP : PHY_STATUS_DOWN);
    
    /* Set the parameter in WanManager */
    rbusError_t rc = rbus_set(handle, param_path, value, NULL);
    rbusValue_Release(value);
    
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to set WanManager PHY status: error=%d\n", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("Successfully notified WanManager: PHY %s on param %s\n", 
             phy_up ? "UP" : "DOWN", param_path);
    return 0;
}

/**
 * @brief Update virtual interface table for WanManager
 * 
 * Searches the WanManager VirtualInterface table for the specified interface
 * and updates its Enable status. Iterates through all table entries to find match.
 * Automatically discovers the correct WanManager interface index on first call.
 * 
 * @param interface_name Interface name (e.g., "veip0")
 * @param is_up true if interface is UP, false if DOWN
 * @return 0 on success, -1 on failure
 * 
 * @note Uses RBUS rbus_get() to query table and rbus_set() to update
 * @note Searches Device.X_RDK_WanManager.Interface.{index}.VirtualInterface table
 * @note Returns error if interface not found in table
 * @note Requires RBUS handle from eponMgr_rbus_get_handle()
 */
int eponMgr_rbus_update_virtual_interface(const char* interface_name, bool is_up) {
    if (!interface_name) {
        EPONMGR_LOG_ERROR("Invalid interface name\n");
        return -1;
    }
    
    EPONMGR_LOG_DEBUG("Updating virtual interface: %s = %s\n", 
              interface_name, is_up ? "UP" : "DOWN");
    
    /* Discover WanManager interface index if not already cached */
    int if_index = discover_wanmanager_interface_index();
    if (if_index < 0) {
        EPONMGR_LOG_ERROR("Failed to discover WanManager interface index\n");
        return -1;
    }
    
    rbusHandle_t handle = (rbusHandle_t)eponMgr_rbus_get_handle();
    if (!handle) {
        EPONMGR_LOG_ERROR("RBUS handle not available\n");
        return -1;
    }
    
    /* Step 1: Query VirtualInterfaceNumberOfEntries to get table size */
    char param_path[256];
    snprintf(param_path, sizeof(param_path), 
             "Device.X_RDK_WanManager.Interface.%d.VirtualInterfaceNumberOfEntries", 
             if_index);
    
    rbusValue_t count_value = NULL;
    rbusError_t rc = rbus_get(handle, param_path, &count_value);
    
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
        snprintf(param_path, sizeof(param_path), 
                 "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.Name", 
                 if_index, i);
        
        /* Query VirtualInterface.{i}.Name */
        rbusValue_t name_value = NULL;
        rc = rbus_get(handle, param_path, &name_value);
        
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
            snprintf(param_path, sizeof(param_path), 
                     "Device.X_RDK_WanManager.Interface.%d.VirtualInterface.%d.Enable", 
                     if_index, i);
            
            rbusValue_t enable_value;
            rbusValue_Init(&enable_value);
            rbusValue_SetBoolean(enable_value, is_up);
            
            rc = rbus_set(handle, param_path, enable_value, NULL);
            rbusValue_Release(enable_value);
            
            if (rc != RBUS_ERROR_SUCCESS) {
                EPONMGR_LOG_ERROR("Failed to set interface %s Enable status: error=%d\n", 
                                 interface_name, rc);
                return -1;
            }
            
            EPONMGR_LOG_INFO("Updated virtual interface %s: Enable=%s on param %s\n", 
                           interface_name, is_up ? "true" : "false", param_path);
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
