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
#include "eponMgr_hal_wrapper.h"

#ifdef USE_DUMMY_RBUS
#include "rbus/eponMgr_rbus_dummy.h"
#else
#include <rbus/rbus.h>
#endif

/* Global RBUS handle */
static rbusHandle_t g_rbus_handle = NULL;
static bool g_rbus_initialized = false;
static eponMgr_hal_wrapper_t *g_hal_wrapper = NULL;



/**
 * @brief Initialize RBUS for EPON Manager
 * 
 * @param component_name Component name for RBUS registration
 * @param hal_wrapper Pointer to HAL wrapper context
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_init(const char* component_name, void *hal_wrapper_ptr) {
    eponMgr_hal_wrapper_t *hal_wrapper = (eponMgr_hal_wrapper_t *)hal_wrapper_ptr;
    
    if (g_rbus_initialized) {
        EPONMGR_LOG_WARN("RBUS already initialized");
        return 0;
    }
    
    if (!component_name || !hal_wrapper) {
        EPONMGR_LOG_ERROR("Invalid parameters");
        return -1;
    }
    
    g_hal_wrapper = hal_wrapper;
    
    EPONMGR_LOG_INFO("Initializing RBUS (component: %s)", component_name);
    
    /* Open RBUS connection */
    rbusError_t rc = rbus_open(&g_rbus_handle, component_name);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to open RBUS: error %d", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("RBUS connection opened successfully");
    
    /* Register TR-181 parameters */
    if (eponMgr_tr181_init(g_rbus_handle, g_hal_wrapper) != 0) {
        EPONMGR_LOG_ERROR("Failed to register TR-181 parameters");
        rbus_close(g_rbus_handle);
        g_rbus_handle = NULL;
        g_hal_wrapper = NULL;
        g_rbus_initialized = false;
        return -1;
    }
    
    g_rbus_initialized = true;
    EPONMGR_LOG_INFO("RBUS initialization complete (%d TR-181 parameters registered)", 
                     eponMgr_tr181_get_param_count());
    
    return 0;
}

/**
 * @brief Cleanup and close RBUS connection
 */
void eponMgr_rbus_cleanup(void) {
    if (!g_rbus_initialized) {
        return;
    }
    
    EPONMGR_LOG_INFO("Cleaning up RBUS");
    
    /* Unregister TR-181 parameters */
    eponMgr_tr181_cleanup(g_rbus_handle);
    
    /* Close RBUS connection */
    if (g_rbus_handle) {
        rbusError_t rc = rbus_close(g_rbus_handle);
        if (rc != RBUS_ERROR_SUCCESS) {
            EPONMGR_LOG_ERROR("Failed to close RBUS: error %d", rc);
        }
        g_rbus_handle = NULL;
    }
    
    g_hal_wrapper = NULL;
    g_rbus_initialized = false;
    EPONMGR_LOG_INFO("RBUS cleanup complete");
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
