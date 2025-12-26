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
#include "../../include/eponMgr_rbus.h"
#include "eponMgr_logger.h"

#ifdef USE_DUMMY_RBUS
#include "../../include/rbus/eponMgr_rbus_dummy.h"
#else
#include <rbus.h>
#endif

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
        EPONMGR_LOG_WARN("RBUS already initialized");
        return 0;
    }
    
    if (!component_name) {
        EPONMGR_LOG_ERROR("Invalid component name");
        return -1;
    }
    
    EPONMGR_LOG_INFO("Initializing RBUS (component: %s)", component_name);
    
    /* Open RBUS connection */
    rbusError_t rc = rbus_open(&g_rbus_handle, component_name);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to open RBUS: error %d", rc);
        return -1;
    }
    
    EPONMGR_LOG_INFO("RBUS connection opened successfully");
    
    /* TODO: Phase 7 - Register TR-181 parameters here */
    /* This will be implemented in Phase 7 with actual parameter handlers */
    EPONMGR_LOG_INFO("TR-181 parameter registration deferred to Phase 7");
    
    g_rbus_initialized = true;
    EPONMGR_LOG_INFO("RBUS initialization complete");
    
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
    
    /* TODO: Phase 7 - Unregister TR-181 parameters here */
    
    /* Close RBUS connection */
    if (g_rbus_handle) {
        rbusError_t rc = rbus_close(g_rbus_handle);
        if (rc != RBUS_ERROR_SUCCESS) {
            EPONMGR_LOG_ERROR("Failed to close RBUS: error %d", rc);
        }
        g_rbus_handle = NULL;
    }
    
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
