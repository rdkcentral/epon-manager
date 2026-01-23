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
 * @file eponMgr_tr181.h
 * @brief TR-181 Parameter Handler for EPON Manager
 * 
 * This module provides TR-181 Device.Optical.Interface parameter handlers
 * for RBUS integration. Maps TR-181 parameters to EPON data context APIs.
 */

#ifndef EPONMGR_TR181_H
#define EPONMGR_TR181_H

#include <stdint.h>
#include <stdbool.h>
#include "eponMgr_data.h"
#include <rbus/rbus.h>

/**
 * @brief Initialize TR-181 parameter handlers and register with RBUS
 * 
 * Uses data context lock/unlock functions for thread-safe access.
 * 
 * @param handle RBUS handle from eponMgr_rbus_init()
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_init(rbusHandle_t handle);

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



/**
 * @brief Synchronize LLID table registrations with current LLID list
 * 
 * Compares current RBUS registrations with actual LLID list state
 * and registers/unregisters instances as needed.
 * Should be called when LLID list changes (add/remove events).
 * 
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_sync_llid_table(void);



/**
 * @brief Synchronize CPE table registrations with current CPE list
 * 
 * Compares current RBUS registrations with actual CPE list state
 * and registers/unregisters instances as needed.
 * Should be called when CPE list changes (add/remove events).
 * 
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_sync_cpe_table(void);



/**
 * @brief Synchronize VEIP Interface table registrations with current interface list
 * 
 * Compares current RBUS registrations with actual interface list state
 * and registers/unregisters instances as needed.
 * Should be called when interface list changes (add/remove/status change events).
 * 
 * @return 0 on success, -1 on failure
 */
int eponMgr_tr181_sync_veip_table(void);

#endif /* EPONMGR_TR181_H */
