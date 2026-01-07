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
 * EPON Manager - Core Data Structure
 * Central data context for EPON Manager with HAL interface and caching
 *
 * Available HAL APIs (from epon_hal.h):
 * - epon_hal_init()
 * - epon_hal_get_link_stats()
 * - epon_hal_get_transceiver_stats()
 * - epon_hal_get_llid_info()
 * - epon_hal_get_manufacturer_info()
 * - epon_hal_get_link_info()
 * - epon_hal_get_interface_list()
 * - epon_hal_get_olt_info()
 * - epon_hal_set_oam_log_mask()
 * - dpoe_hal_get_cpe_mac_table()
 */

#ifndef EPONMGR_DATA_H
#define EPONMGR_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "epon_hal.h"
#include "eponMgr_statsData.h"
#include "eponMgr_onu_state.h"

/**
 * @brief Core EPON Manager data context
 * 
 * This structure holds all the central state and data for the EPON Manager:
 * - Statistics cache with TTL
 * - Direct HAL data structures
 * - ONU state management (cached info like OLT, manufacturer, link info)
 * - HAL initialization state and configuration
 * - Thread-safety through internal mutex
 */
typedef struct {
    // Statistics data storage (with TTL)
    eponMgr_statsData_t *stats_data;
    
    // Direct HAL data structures - no additional wrapper layers
    epon_interface_list_t interface_list;
    epon_llid_list_t llid_list;
    dpoe_cpe_mac_table_t cpe_table;
    
    // Change tracking for TR-181 sync optimization
    uint32_t if_list_count_cache;
    uint32_t llid_count_cache;
    uint32_t cpe_count_cache;
    
    // ONU state (cached info that rarely changes)
    eponMgr_onu_state_t *onu_state;
    
    // HAL initialization state
    bool hal_initialized;
    epon_hal_config_t hal_config;
    
    // Thread safety
    pthread_mutex_t mutex;
} eponMgr_data_t;

/**
 * @brief Initialize EPON Manager data context
 * @param eponData Pointer to data context
 * @param config HAL configuration with callbacks
 * @param cache_ttl Cache TTL in seconds (default 30)
 * @return 0 on success, -1 on error
 */
int eponMgr_data_init(eponMgr_data_t *eponData, 
                      epon_hal_config_t *config, 
                      uint32_t cache_ttl);

/**
 * @brief Destroy EPON Manager data context and cleanup resources
 * @param eponData Pointer to data context
 */
void eponMgr_data_destroy(eponMgr_data_t *eponData);

/**
 * @brief Lock global data context for thread-safe access
 * 
 * Must be followed by eponMgr_data_unlock()
 * Returns pointer to locked data context
 * 
 * @return Pointer to locked data context, NULL if not initialized
 */
eponMgr_data_t* eponMgr_data_lock(void);

/**
 * @brief Unlock global data context after use
 */
void eponMgr_data_unlock(void);

/**
 * @brief Get EPON HAL API version
 * @return API version
 */
uint32_t eponMgr_data_get_hal_version(void);

/**
 * @brief Initialize EPON HAL (cached in onu_state)
 * @param eponData Pointer to data context
 * @return EPON HAL return code
 */
int eponMgr_data_hal_init(eponMgr_data_t *eponData);

/**
 * @brief Get link statistics (with 30s cache)
 * @param eponData Pointer to data context
 * @param stats Pointer to stats structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_data_get_link_stats(eponMgr_data_t *eponData,
                                 epon_hal_link_stats_t *stats);

/**
 * @brief Get transceiver statistics (with 30s cache)
 * @param eponData Pointer to data context
 * @param stats Pointer to stats structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_data_get_transceiver_stats(eponMgr_data_t *eponData,
                                        epon_hal_transceiver_stats_t *stats);

/**
 * @brief Get LLID list (fills HAL structure directly)
 * @param eponData Pointer to data context
 * @param llid_list Pointer to LLID list structure (HAL fills)
 * @return EPON HAL return code
 */
int eponMgr_data_get_llid_info(eponMgr_data_t *eponData,
                                epon_llid_list_t *llid_list);

/**
 * @brief Get interface list (fills HAL structure directly)
 * @param eponData Pointer to data context
 * @param if_list Pointer to interface list structure (HAL fills)
 * @return EPON HAL return code
 */
int eponMgr_data_get_interface_list(eponMgr_data_t *eponData,
                                     epon_interface_list_t *if_list);

/**
 * @brief Get OLT information (cached with validity flag)
 * @param eponData Pointer to data context
 * @param olt_info Pointer to OLT info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_data_get_olt_info(eponMgr_data_t *eponData,
                               epon_olt_info_t *olt_info);

/**
 * @brief Get ONU manufacturer information (cached with validity flag)
 * @param eponData Pointer to data context
 * @param mfr_info Pointer to manufacturer info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_data_get_onu_manufacturer_info(eponMgr_data_t *eponData,
                                            epon_onu_manufacturer_info_t *mfr_info);

/**
 * @brief Get link information (cached with validity flag)
 * @param eponData Pointer to data context
 * @param link_info Pointer to link info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_data_get_link_info(eponMgr_data_t *eponData,
                                epon_hal_link_info_t *link_info);

/**
 * @brief Get maximum CPE count
 * @param eponData Pointer to data context
 * @param max_cpe Pointer to max CPE count
 * @return EPON HAL return code
 */
int eponMgr_data_get_max_cpe(eponMgr_data_t *eponData,
                              uint32_t *max_cpe);

/**
 * @brief Get CPE MAC address table (fills HAL structure directly)
 * @param eponData Pointer to data context
 * @param cpe_table Pointer to CPE table (HAL fills)
 * @return EPON HAL return code
 */
int eponMgr_data_get_cpe_mac_table(eponMgr_data_t *eponData,
                                    dpoe_cpe_mac_table_t *cpe_table);

/**
 * @brief Set OAM log level (no caching)
 * @param eponData Pointer to data context
 * @param log_level Log level bitmask
 * @return EPON HAL return code
 */
int eponMgr_data_set_oam_log_level(eponMgr_data_t *eponData,
                                    uint32_t log_level);

/**
 * @brief Invalidate all cache entries (call on ONU status change)
 * @param eponData Pointer to data context
 */
void eponMgr_data_invalidate_cache(eponMgr_data_t *eponData);

/**
 * @brief Get LLID entry by index (for TR-181 access)
 * Must call eponMgr_data_get_llid_info() first to populate data
 * @param eponData Pointer to data context
 * @param index Zero-based index
 * @param llid_info Output llid info
 * @return 0 on success, -1 on error
 */
int eponMgr_data_get_llid_at_index(eponMgr_data_t *eponData, uint32_t index, epon_llid_info_t *llid_info);

/**
 * @brief Get CPE entry by index (for TR-181 access)
 * Must call eponMgr_data_get_cpe_mac_table() first to populate data
 * @param eponData Pointer to data context
 * @param index Zero-based index
 * @param cpe_entry Output CPE entry
 * @return 0 on success, -1 on error
 */
int eponMgr_data_get_cpe_at_index(eponMgr_data_t *eponData, uint32_t index, dpoe_cpe_mac_entry_t *cpe_entry);

/**
 * @brief Get interface entry by index (for TR-181 access)
 * Must call eponMgr_data_get_interface_list() first to populate data
 * @param eponData Pointer to data context
 * @param index Zero-based index
 * @param if_info Output interface info
 * @return 0 on success, -1 on error
 */
int eponMgr_data_get_interface_at_index(eponMgr_data_t *eponData, uint32_t index, epon_onu_interface_info_t *if_info);

/**
 * @brief Get interface entry by name (for TR-181 access)
 * Must call eponMgr_data_get_interface_list() first to populate data
 * @param eponData Pointer to data context
 * @param name Interface name
 * @param if_info Output interface info
 * @return 0 on success, -1 on error
 */
int eponMgr_data_get_interface_by_name(eponMgr_data_t *eponData, const char *name, epon_onu_interface_info_t *if_info);

/**
 * @brief Get LLID count (cached from last HAL call)
 * @param eponData Pointer to data context
 * @return LLID count
 */
static inline uint32_t eponMgr_data_get_llid_count(eponMgr_data_t *eponData) {
    return eponData ? eponData->llid_list.llid_count : 0;
}

/**
 * @brief Get CPE count (cached from last HAL call)
 * @param eponData Pointer to data context
 * @return CPE count
 */
static inline uint32_t eponMgr_data_get_cpe_count(eponMgr_data_t *eponData) {
    if (!eponData) return 0;
    return eponData->cpe_table.static_cpe_count + eponData->cpe_table.dynamic_cpe_count;
}

/**
 * @brief Get interface count (cached from last HAL call)
 * @param eponData Pointer to data context
 * @return Interface count
 */
static inline uint32_t eponMgr_data_get_interface_count(eponMgr_data_t *eponData) {
    return eponData ? eponData->interface_list.interface_count : 0;
}

#endif /* EPONMGR_DATA_H */
