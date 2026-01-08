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

/*
 * EPON Manager - ONU State Management
 * Thread-safe ONU state tracking and coordination
 */

#ifndef EPONMGR_ONU_STATE_H
#define EPONMGR_ONU_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include "epon_hal.h"

/**
 * @brief ONU state manager structure
 * 
 * Tracks the overall ONU state including:
 * - Current ONU status (LOS, registration, etc.)
 * - OLT information (learned during registration)
 * - Manufacturer information
 * - Link information (mode, encryption)
 * - DPoE support status
 * - Timestamps for state changes
 */
typedef struct {
    /* Current state */
    epon_onu_status_t current_status;           /**< Current ONU status */
    epon_onu_status_t previous_status;          /**< Previous ONU status */
    time_t last_status_change;                  /**< Timestamp of last status change */
    
    /* OLT information (learned during registration) */
    epon_olt_info_t olt_info;                   /**< OLT information */
    bool olt_info_valid;                        /**< OLT info validity flag */
    
    /* ONU manufacturer information */
    epon_onu_manufacturer_info_t manufacturer_info;  /**< Manufacturer info */
    bool manufacturer_info_valid;               /**< Manufacturer info validity flag */
    
    /* Link configuration */
    epon_hal_link_info_t link_info;             /**< Link configuration */
    bool link_info_valid;                       /**< Link info validity flag */
    
    /* DPoE support */
    bool dpoe_supported;                        /**< DPoE support enabled */
    
    /* HAL initialization state */
    bool hal_initialized;                       /**< HAL has been initialized */
    
    /* Thread safety */
    pthread_mutex_t mutex;                      /**< Mutex for thread-safe access */
} eponMgr_onu_state_t;

/**
 * @brief Initialize ONU state manager
 * 
 * @param state Pointer to ONU state structure
 * @param dpoe_supported DPoE support flag
 * @return 0 on success, -1 on error
 */
int eponMgr_onu_state_init(eponMgr_onu_state_t *state, bool dpoe_supported);

/**
 * @brief Destroy ONU state manager and cleanup resources
 * 
 * @param state Pointer to ONU state structure
 */
void eponMgr_onu_state_destroy(eponMgr_onu_state_t *state);

/**
 * @brief Update ONU status
 * 
 * This function should be called from the HAL status callback.
 * It updates the current status and tracks previous status.
 * 
 * @param state Pointer to ONU state structure
 * @param new_status New ONU status
 * @return 0 on success, -1 on error
 */
int eponMgr_onu_state_update_status(eponMgr_onu_state_t *state, 
                                     epon_onu_status_t new_status);

/**
 * @brief Invalidate all cached information
 * 
 * Called when ONU status changes to invalidate all cached data.
 * 
 * @param state Pointer to ONU state structure
 */
void eponMgr_onu_state_invalidate_all(eponMgr_onu_state_t *state);

/**
 * @brief Mark HAL as initialized
 * 
 * @param state Pointer to ONU state structure
 */
void eponMgr_onu_state_set_hal_initialized(eponMgr_onu_state_t *state);

/**
 * @brief Check if HAL is initialized
 * 
 * @param state Pointer to ONU state structure
 * @return true if HAL is initialized, false otherwise
 */
bool eponMgr_onu_state_is_hal_initialized(eponMgr_onu_state_t *state);

#endif /* EPONMGR_ONU_STATE_H */
