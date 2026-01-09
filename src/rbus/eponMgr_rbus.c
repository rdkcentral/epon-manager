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
#include <rbus/rbus.h>

/* Global RBUS handle */
static rbusHandle_t g_rbus_handle = NULL;
static bool g_rbus_initialized = false;



/**
 * @brief Initialize RBUS for EPON Manager
 * 
 * @param component_name Component name for RBUS registration
 * @return 0 on success, -1 on failure
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
 */
void eponMgr_rbus_cleanup(void) {
    if (!g_rbus_initialized) {
        return;
    }
    
    EPONMGR_LOG_INFO("Cleaning up RBUS\n");
    
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
 * Called after HAL is initialized
 * 
 * @return 0 on success, -1 on failure
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
    
    return 0;
}

/**
 * @brief Get RBUS handle for direct use
 * 
 * @return RBUS handle or NULL if not initialized
 */
eponMgr_rbus_handle_t eponMgr_rbus_get_handle(void) {
    if (!g_rbus_initialized) {
        return NULL;
    }
    return (eponMgr_rbus_handle_t)g_rbus_handle;
}
