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
 * EPON Manager - Core Data Structure Implementation
 * Central data context for EPON Manager with HAL interface and caching
 */

/* Include logger FIRST to enable HAL logging via HAL_LOG_FUNCTION */
#include "eponMgr_logger.h"

#include "eponMgr_data.h"
#include "eponMgr_tr181.h"
#include <string.h>
#include <stdlib.h>

// Global EPON data context
static eponMgr_data_t *g_eponData = NULL;

int eponMgr_data_init(eponMgr_data_t *eponData, 
                      epon_hal_config_t *config, 
                      uint32_t cache_ttl)
{
    if (!eponData || !config) return -1;
    
    EPONMGR_LOG_INFO("Initializing core data context with cache TTL: %u seconds\n", cache_ttl);
    
    memset(eponData, 0, sizeof(eponMgr_data_t));
    
    // Initialize statistics cache
    eponData->stats_data = (eponMgr_statsData_t *)malloc(sizeof(eponMgr_statsData_t));
    if (!eponData->stats_data) return -1;

    if (eponMgr_statsData_init(eponData->stats_data, cache_ttl) != 0) {
        free(eponData->stats_data);
        return -1;
    }
    
    // Initialize ONU state (for cached info like OLT, manufacturer, link)
    eponData->onu_state = (eponMgr_onu_state_t *)malloc(sizeof(eponMgr_onu_state_t));
    if (!eponData->onu_state) {
        eponMgr_statsData_destroy(eponData->stats_data);
        free(eponData->stats_data);
        return -1;
    }
    if (eponMgr_onu_state_init(eponData->onu_state, config->dpoe_supported) != 0) {
        free(eponData->onu_state);
        eponMgr_statsData_destroy(eponData->stats_data);
        free(eponData->stats_data);
        return -1;
    }
    
    // Initialize direct HAL structures (zero-init already done by memset)
    // They will be populated on first HAL call
    
    // Store HAL config
    memcpy(&eponData->hal_config, config, sizeof(epon_hal_config_t));
    
    // Initialize recursive mutex to allow nested locks from same thread
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&eponData->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    eponData->hal_initialized = false;
    
    // Store global reference
    g_eponData = eponData;
    
    EPONMGR_LOG_INFO("Core data context initialized successfully\n");
    return 0;
}

void eponMgr_data_destroy(eponMgr_data_t *eponData)
{
    if (!eponData) return;
    
    EPONMGR_LOG_INFO("Destroying core data context\n");
    
    // Clear global reference
    if (g_eponData == eponData) {
        g_eponData = NULL;
    }
    
    pthread_mutex_lock(&eponData->mutex);
    
    if (eponData->onu_state) {
        eponMgr_onu_state_destroy(eponData->onu_state);
        free(eponData->onu_state);
    }
    
    // Free dynamic memory in HAL structures if allocated
    if (eponData->llid_list.llid_list) {
        free(eponData->llid_list.llid_list);
    }
    if (eponData->cpe_table.cpe_list) {
        free(eponData->cpe_table.cpe_list);
    }
    
    if (eponData->stats_data) {
        eponMgr_statsData_destroy(eponData->stats_data);
        free(eponData->stats_data);
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    pthread_mutex_destroy(&eponData->mutex);
    
    EPONMGR_LOG_INFO("Core data context destroyed\n");
}

eponMgr_data_t* eponMgr_data_lock(void)
{
    if (!g_eponData) return NULL;
    pthread_mutex_lock(&g_eponData->mutex);
    return g_eponData;
}

void eponMgr_data_unlock(void)
{
    if (!g_eponData) return;
    pthread_mutex_unlock(&g_eponData->mutex);
}

uint32_t eponMgr_data_get_hal_version(void)
{
    return epon_hal_get_version();
}

int eponMgr_data_hal_init(eponMgr_data_t *eponData)
{
    if (!eponData) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Initializing EPON HAL\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    int ret = epon_hal_init(&eponData->hal_config);
    if (ret == EPON_HAL_SUCCESS) {
        eponData->hal_initialized = true;
        eponMgr_onu_state_set_hal_initialized(eponData->onu_state);
        EPONMGR_LOG_INFO("EPON HAL initialized successfully\n");
    } else {
        EPONMGR_LOG_INFO("EPON HAL initialization failed with error code: %d\n", ret);
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

const epon_hal_link_stats_t* eponMgr_data_get_link_stats(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Check if cache is valid (TTL check)
    if (eponData->stats_data->link_stats.valid && 
        eponMgr_statsData_is_stats_valid(eponData->stats_data->link_stats.timestamp, 
                                          eponData->stats_data->ttl_seconds)) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_DEBUG("Link statistics retrieved from cache\n");
        return &eponData->stats_data->link_stats.data;
    }
    
    // Cache invalid - call HAL to fill directly
    epon_hal_link_stats_t* ptr = &eponData->stats_data->link_stats.data;
    ptr->struct_size = sizeof(epon_hal_link_stats_t);
    
    int ret = epon_hal_get_link_stats(ptr);
    if (ret == EPON_HAL_SUCCESS) {
        // Update timestamp and validity
        eponData->stats_data->link_stats.timestamp = time(NULL);
        eponData->stats_data->link_stats.valid = true;
        EPONMGR_LOG_INFO("Link statistics retrieved from HAL and cached\n");
        pthread_mutex_unlock(&eponData->mutex);
        return ptr;
    } else {
        EPONMGR_LOG_INFO("Failed to get link statistics from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return NULL;
}

const epon_hal_transceiver_stats_t* eponMgr_data_get_transceiver_stats(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Check if cache is valid (TTL check)
    if (eponData->stats_data->transceiver_stats.valid && 
        eponMgr_statsData_is_stats_valid(eponData->stats_data->transceiver_stats.timestamp, 
                                          eponData->stats_data->ttl_seconds)) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_DEBUG("Transceiver statistics retrieved from cache\n");
        return &eponData->stats_data->transceiver_stats.data;
    }
    
    // Cache invalid - call HAL to fill directly
    epon_hal_transceiver_stats_t* ptr = &eponData->stats_data->transceiver_stats.data;
    ptr->struct_size = sizeof(epon_hal_transceiver_stats_t);
    
    int ret = epon_hal_get_transceiver_stats(ptr);
    if (ret == EPON_HAL_SUCCESS) {
        // Update timestamp and validity
        eponData->stats_data->transceiver_stats.timestamp = time(NULL);
        eponData->stats_data->transceiver_stats.valid = true;
        EPONMGR_LOG_INFO("Transceiver statistics retrieved from HAL and cached\n");
        pthread_mutex_unlock(&eponData->mutex);
        return ptr;
    } else {
        EPONMGR_LOG_INFO("Failed to get transceiver statistics from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return NULL;
}

const epon_llid_list_t* eponMgr_data_get_llid_info(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    EPONMGR_LOG_DEBUG("Getting LLID information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Call HAL to fill owned LLID list directly (zero-copy)
    int ret = epon_hal_get_llid_info(&eponData->llid_list);
    if (ret == EPON_HAL_SUCCESS) {
        // Check if count changed for TR-181 sync
        if (eponData->llid_list.llid_count != eponData->llid_count_cache) {
            eponData->llid_count_cache = eponData->llid_list.llid_count;
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("LLID count changed to %u, updating TR-181\n", eponData->llid_list.llid_count);
            eponMgr_tr181_sync_llid_table();
            return &eponData->llid_list;
        }
        pthread_mutex_unlock(&eponData->mutex);
        return &eponData->llid_list;
    } else {
        EPONMGR_LOG_WARN("Failed to get LLID information from HAL\n");
        pthread_mutex_unlock(&eponData->mutex);
        return NULL;
    }
}

const epon_interface_list_t* eponMgr_data_get_interface_list(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    EPONMGR_LOG_INFO("Getting interface list\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Call HAL to fill owned interface list directly (zero-copy)
    int ret = epon_hal_get_interface_list(&eponData->interface_list);
    if (ret == EPON_HAL_SUCCESS) {
        EPONMGR_LOG_INFO("Retrieved %u interface(s) from HAL\n", eponData->interface_list.interface_count);
        
        // Check if count changed for TR-181 sync
        if (eponData->interface_list.interface_count != eponData->if_list_count_cache) {
            eponData->if_list_count_cache = eponData->interface_list.interface_count;
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("Interface count changed to %u, updating TR-181\n", eponData->interface_list.interface_count);
            eponMgr_tr181_sync_veip_table();
            return &eponData->interface_list;
        }
        pthread_mutex_unlock(&eponData->mutex);
        return &eponData->interface_list;
    } else {
        EPONMGR_LOG_WARN("Failed to get interface list from HAL\n");
        pthread_mutex_unlock(&eponData->mutex);
        return NULL;
    }
}

const epon_olt_info_t* eponMgr_data_get_olt_info(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Check if cache is valid
    if (eponData->onu_state->olt_info_valid) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_DEBUG("OLT information retrieved from cache\n");
        return &eponData->onu_state->olt_info;
    }
    
    // Cache invalid - call HAL to fill directly
    epon_olt_info_t* ptr = &eponData->onu_state->olt_info;
    ptr->struct_size = sizeof(epon_olt_info_t);
    
    int ret = epon_hal_get_olt_info(ptr);
    if (ret == EPON_HAL_SUCCESS) {
        eponData->onu_state->olt_info_valid = true;
        EPONMGR_LOG_INFO("OLT information retrieved from HAL and cached\n");
        pthread_mutex_unlock(&eponData->mutex);
        return ptr;
    } else {
        EPONMGR_LOG_INFO("Failed to get OLT information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return NULL;
}

const epon_onu_manufacturer_info_t* eponMgr_data_get_onu_manufacturer_info(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Check if cache is valid
    if (eponData->onu_state->manufacturer_info_valid) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_DEBUG("ONU manufacturer information retrieved from cache\n");
        return &eponData->onu_state->manufacturer_info;
    }
    
    // Cache invalid - call HAL to fill directly
    epon_onu_manufacturer_info_t* ptr = &eponData->onu_state->manufacturer_info;
    ptr->struct_size = sizeof(epon_onu_manufacturer_info_t);
    
    int ret = epon_hal_get_manufacturer_info(ptr);
    if (ret == EPON_HAL_SUCCESS) {
        eponData->onu_state->manufacturer_info_valid = true;
        EPONMGR_LOG_INFO("ONU manufacturer information retrieved from HAL and cached\n");
        pthread_mutex_unlock(&eponData->mutex);
        return ptr;
    } else {
        EPONMGR_LOG_INFO("Failed to get ONU manufacturer information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return NULL;
}

const epon_hal_link_info_t* eponMgr_data_get_link_info(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Check if cache is valid
    if (eponData->onu_state->link_info_valid) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_DEBUG("Link information retrieved from cache\n");
        return &eponData->onu_state->link_info;
    }
    
    // Cache invalid - call HAL to fill directly
    epon_hal_link_info_t* ptr = &eponData->onu_state->link_info;
    
    int ret = epon_hal_get_link_info(ptr);
    if (ret == EPON_HAL_SUCCESS) {
        eponData->onu_state->link_info_valid = true;
        EPONMGR_LOG_INFO("Link information retrieved from HAL and cached\n");
        pthread_mutex_unlock(&eponData->mutex);
        return ptr;
    } else {
        EPONMGR_LOG_INFO("Failed to get link information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return NULL;
}

int eponMgr_data_get_max_cpe(eponMgr_data_t *eponData,
                              uint32_t *max_cpe)
{
    if (!eponData || !max_cpe) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting maximum CPE count\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Return from CPE table
    *max_cpe = eponData->cpe_table.max_cpe;
    
    EPONMGR_LOG_INFO("Maximum CPE count: %u\n", *max_cpe);
    
    pthread_mutex_unlock(&eponData->mutex);
    return EPON_HAL_SUCCESS;
}

const dpoe_cpe_mac_table_t* eponMgr_data_get_cpe_mac_table(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Call HAL to fill owned CPE table directly (zero-copy)
    int ret = dpoe_hal_get_cpe_mac_table(&eponData->cpe_table);
    if (ret == EPON_HAL_SUCCESS) {
        uint32_t total = eponData->cpe_table.static_cpe_count + eponData->cpe_table.dynamic_cpe_count;
        EPONMGR_LOG_DEBUG("Retrieved CPE MAC table: %u static, %u dynamic entries\n",
                        eponData->cpe_table.static_cpe_count, eponData->cpe_table.dynamic_cpe_count);
        
        // Check if count changed for TR-181 sync
        if (total != eponData->cpe_count_cache) {
            eponData->cpe_count_cache = total;
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("CPE count changed to %u, updating TR-181\n", total);
            eponMgr_tr181_sync_cpe_table();
            return &eponData->cpe_table;
        }
        pthread_mutex_unlock(&eponData->mutex);
        return &eponData->cpe_table;
    } else {
        EPONMGR_LOG_WARN("Failed to get CPE MAC table from HAL\n");
        pthread_mutex_unlock(&eponData->mutex);
        return NULL;
    }
}

int eponMgr_data_set_oam_log_level(eponMgr_data_t *eponData,
                                    uint32_t log_level)
{
    if (!eponData) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Setting OAM log level: 0x%08x\n", log_level);
    
    // No caching for log level settings
    return epon_hal_set_oam_log_mask(log_level);
}

void eponMgr_data_invalidate_cache(eponMgr_data_t *eponData)
{
    if (!eponData) return;
    
    EPONMGR_LOG_INFO("Invalidating all cached data\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Invalidate all storage entries (both statistics and info)
    eponMgr_statsData_invalidate_all(eponData->stats_data);
    eponMgr_onu_state_invalidate_all(eponData->onu_state);
    
    // Reset count caches to force TR-181 sync on next call
    eponData->if_list_count_cache = 0;
    eponData->llid_count_cache = 0;
    eponData->cpe_count_cache = 0;
    
    pthread_mutex_unlock(&eponData->mutex);
}

int eponMgr_data_get_llid_at_index(eponMgr_data_t *eponData, uint32_t index, epon_llid_info_t *llid_info)
{
    if (!eponData || !llid_info) return -1;
    
    pthread_mutex_lock(&eponData->mutex);
    
    if (index >= eponData->llid_list.llid_count || !eponData->llid_list.llid_list) {
        pthread_mutex_unlock(&eponData->mutex);
        return -1;
    }
    
    *llid_info = eponData->llid_list.llid_list[index];
    pthread_mutex_unlock(&eponData->mutex);
    return 0;
}

int eponMgr_data_get_cpe_at_index(eponMgr_data_t *eponData, uint32_t index, dpoe_cpe_mac_entry_t *cpe_entry)
{
    if (!eponData || !cpe_entry) return -1;
    
    pthread_mutex_lock(&eponData->mutex);
    
    uint32_t total = eponData->cpe_table.static_cpe_count + eponData->cpe_table.dynamic_cpe_count;
    if (index >= total || !eponData->cpe_table.cpe_list) {
        pthread_mutex_unlock(&eponData->mutex);
        return -1;
    }
    
    *cpe_entry = eponData->cpe_table.cpe_list[index];
    pthread_mutex_unlock(&eponData->mutex);
    return 0;
}

int eponMgr_data_get_interface_at_index(eponMgr_data_t *eponData, uint32_t index, epon_onu_interface_info_t *if_info)
{
    if (!eponData || !if_info) return -1;
    
    pthread_mutex_lock(&eponData->mutex);
    
    if (index >= eponData->interface_list.interface_count) {
        pthread_mutex_unlock(&eponData->mutex);
        return -1;
    }
    
    *if_info = eponData->interface_list.interface[index];
    pthread_mutex_unlock(&eponData->mutex);
    return 0;
}

int eponMgr_data_get_interface_by_name(eponMgr_data_t *eponData, const char *name, epon_onu_interface_info_t *if_info)
{
    if (!eponData || !name || !if_info) return -1;
    
    pthread_mutex_lock(&eponData->mutex);
    
    for (uint32_t i = 0; i < eponData->interface_list.interface_count; i++) {
        if (strncmp(eponData->interface_list.interface[i].name, name, EPON_HAL_INTERFACE_NAME_LEN) == 0) {
            *if_info = eponData->interface_list.interface[i];
            pthread_mutex_unlock(&eponData->mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return -1;
}
