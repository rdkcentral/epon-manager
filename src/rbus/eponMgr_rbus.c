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
 * @file eponMgr_rbus.c
 * @brief EPON Manager RBUS Integration
 * 
 * This module provides the main RBUS integration including:
 * - RBUS initialization and cleanup
 * - TR-181 parameter registration
 * - Integration with WanManager notifications
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "eponMgr_rbus.h"
#include "eponMgr_tr181.h"
#include "eponMgr_logger.h"
#include "eponMgr_controller.h"
#include "eponMgr_telemetry_debug.h" /* TEMP: remove before merging PR */
#include <rbus/rbus.h>

/* Global RBUS handle */
static rbusHandle_t g_rbus_handle = NULL;
static bool g_rbus_initialized = false;



/**
 * @brief Initialize RBUS for EPON Manager
 * 
 * Opens RBUS connection with the specified component name. This must be called
 * before any other RBUS operations including TR-181 registration and PSM access.
 * 
 * @param component_name Component name for RBUS registration (e.g., "epon_manager")
 * @return 0 on success, -1 on failure
 * 
 * @note Sets global RBUS handle for all RBUS operations
 * @note Returns success if already initialized
 * @note Must be called before PSM operations
 */
int eponMgr_rbus_init(const char* component_name) {
    
    if (g_rbus_initialized) {
        EPONMGR_LOG_WARN("RBUS already initialized\n");
        return 0;
    }
    
    if (!component_name) {
        EPONMGR_LOG_ERROR("Invalid component name\n");
        return -1;
    }
    
    EPONMGR_LOG_INFO("Initializing RBUS (component: %s)\n", component_name);
    
    /* Open RBUS connection */
    rbusError_t rc = rbus_open(&g_rbus_handle, component_name);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to open RBUS: error %d\n", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("RBUS connection opened successfully\n");
    g_rbus_initialized = true;
    
    return 0;
}

/**
 * @brief Cleanup and close RBUS connection
 * 
 * Unregisters TR-181 parameters and closes the RBUS connection. Should be
 * called during shutdown to cleanup resources.
 * 
 * @note Safe to call if not initialized (no-op)
 * @note Automatically unregisters TR-181 parameters first
 * @note Clears global RBUS handle
 */
void eponMgr_rbus_cleanup(void) {
    if (!g_rbus_initialized) {
        return;
    }
    
    EPONMGR_LOG_INFO("Cleaning up RBUS\n");
    
    /* TEMP: unregister debug telemetry trigger DML - remove before merging PR */
    eponMgr_telemetry_debug_cleanup(g_rbus_handle);

    /* Unregister TR-181 parameters */
    eponMgr_tr181_cleanup(g_rbus_handle);
    
    /* Close RBUS connection */
    if (g_rbus_handle) {
        rbusError_t rc = rbus_close(g_rbus_handle);
        if (rc != RBUS_ERROR_SUCCESS) {
            EPONMGR_LOG_ERROR("Failed to close RBUS: error %d\n", rc);
        }
        g_rbus_handle = NULL;
    }
    
    g_rbus_initialized = false;
    EPONMGR_LOG_INFO("RBUS cleanup complete\n");
}

/**
 * @brief Register TR-181 parameters
 * 
 * Registers all TR-181 Device.Optical.Interface parameters with RBUS.
 * Should be called after HAL is initialized so data context is available.
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Requires RBUS to be initialized first
 * @note Should be called after HAL initialization
 * @note Registers both static and dynamic table parameters
 */
int eponMgr_rbus_register_tr181(void) {
    if (!g_rbus_initialized) {
        EPONMGR_LOG_ERROR("RBUS not initialized\n");
        return -1;
    }
    
    /* Register TR-181 parameters */
    if (eponMgr_tr181_init(g_rbus_handle) != 0) {
        EPONMGR_LOG_ERROR("Failed to register TR-181 parameters\n");
        return -1;
    }
    
    EPONMGR_LOG_INFO("TR-181 parameters registered (%d parameters)\n",
                     eponMgr_tr181_get_param_count());

    /* TEMP: register debug telemetry trigger DML - remove before merging PR */
    if (eponMgr_telemetry_debug_init(g_rbus_handle) != 0) {
        EPONMGR_LOG_WARN("Failed to register debug telemetry DML (non-fatal)\n");
    }

    return 0;
}

/**
 * @brief Get RBUS handle for direct use
 * 
 * Returns the global RBUS handle for direct RBUS API calls. Used by
 * PSM module and WanManager update functions.
 * 
 * @return RBUS handle or NULL if not initialized
 * 
 * @note Returns opaque handle that can be cast to rbusHandle_t
 * @note Handle is managed by this module - do not close directly
 */
eponMgr_rbus_handle_t eponMgr_rbus_get_handle(void) {
    if (!g_rbus_initialized) {
        return NULL;
    }
    return (eponMgr_rbus_handle_t)g_rbus_handle;
}
