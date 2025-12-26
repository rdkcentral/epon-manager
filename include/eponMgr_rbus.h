/**
 * @file eponMgr_rbus.h
 * @brief EPON Manager RBUS Integration API
 * 
 * This module provides RBUS integration for EPON Manager including:
 * - TR-181 parameter registration and handlers
 * - WanManager notifications
 * - RBUS initialization and cleanup
 */

#ifndef EPON_MGR_RBUS_H
#define EPON_MGR_RBUS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RBUS handle (opaque)
 */
typedef void* eponMgr_rbus_handle_t;

/**
 * @brief Initialize RBUS for EPON Manager
 * 
 * @param component_name Component name for RBUS registration
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_init(const char* component_name);

/**
 * @brief Cleanup and close RBUS connection
 */
void eponMgr_rbus_cleanup(void);

/**
 * @brief Notify WanManager of PHY status change
 * 
 * This function is called when the EPON PHY status changes based on
 * interface states:
 * - PHY UP: At least one interface is UP
 * - PHY DOWN: All interfaces are DOWN
 * 
 * @param phy_up true if PHY should be reported as UP, false for DOWN
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_notify_wanmanager_phy_status(bool phy_up);

/**
 * @brief Update virtual interface table for WanManager
 * 
 * @param interface_name Interface name (e.g., "veip0")
 * @param is_up true if interface is UP, false if DOWN
 * @return 0 on success, -1 on failure
 */
int eponMgr_rbus_update_virtual_interface(const char* interface_name, bool is_up);

/**
 * @brief Get RBUS handle for direct use
 * 
 * @return RBUS handle or NULL if not initialized
 */
eponMgr_rbus_handle_t eponMgr_rbus_get_handle(void);

#ifdef __cplusplus
}
#endif

#endif /* EPON_MGR_RBUS_H */
