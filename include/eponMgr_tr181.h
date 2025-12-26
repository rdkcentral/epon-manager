/**
 * @file eponMgr_tr181.h
 * @brief TR-181 Parameter Handler for EPON Manager
 * 
 * This module provides TR-181 Device.Optical.Interface parameter handlers
 * for RBUS integration. Maps TR-181 parameters to HAL wrapper APIs.
 */

#ifndef EPONMGR_TR181_H
#define EPONMGR_TR181_H

#include <stdint.h>
#include <stdbool.h>
#include "../src/core/hal_wrapper/eponMgr_hal_wrapper.h"

#ifdef USE_DUMMY_RBUS
#include "rbus/eponMgr_rbus_dummy.h"
#else
#include <rbus.h>
#endif

/**
 * @brief Initialize TR-181 parameter handlers and register with RBUS
 * 
 * @param handle RBUS handle from eponMgr_rbus_init()
 * @param hal_wrapper Pointer to HAL wrapper context (for data access)
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_init(rbusHandle_t handle, eponMgr_hal_wrapper_t *hal_wrapper);

/**
 * @brief Cleanup TR-181 parameter handlers and unregister from RBUS
 * 
 * @param handle RBUS handle
 */
void eponMgr_tr181_cleanup(rbusHandle_t handle);

/**
 * @brief Get number of TR-181 parameters registered
 * 
 * @return Number of registered parameters (base + dynamic tables)
 */
int eponMgr_tr181_get_param_count(void);

#endif /* EPONMGR_TR181_H */
