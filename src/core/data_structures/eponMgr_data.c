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

/**
 * @brief Initialize EPON Manager data context
 * 
 * Initializes the central EPON Manager data context with statistics cache,
 * ONU state management, and thread-safety primitives. Allocates and initializes
 * sub-structures including stats data and ONU state.
 * 
 * @param eponData Pointer to data context structure
 * @param config HAL configuration with callbacks and DPoE support flag
 * @param cache_ttl Cache TTL in seconds (default 30)
 * @return 0 on success, -1 on error
 * 
 * @note Caller must call eponMgr_data_destroy() to cleanup resources
 * @note Uses recursive mutex to allow nested locks from same thread
 * @note Sets global data context pointer for lock/unlock functions
 */
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

/**
 * @brief Destroy EPON Manager data context and cleanup resources
 * 
 * Destroys the data context, frees all allocated memory including statistics data,
 * ONU state, and dynamically allocated HAL structures. Clears the global data
 * context pointer.
 * 
 * @param eponData Pointer to data context
 * 
 * @note Safe to call with NULL pointer
 * @note Locks mutex before cleanup, then destroys mutex
 * @note Clears global reference if this was the global context
 */
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

/**
 * @brief Lock global data context for thread-safe access
 * 
 * Acquires the global data context mutex and returns a pointer to the locked
 * context. This provides thread-safe access to the global EPON data.
 * 
 * @return Pointer to locked data context, NULL if not initialized
 * 
 * @note Caller MUST call eponMgr_data_unlock() after use
 * @note Uses recursive mutex - same thread can lock multiple times
 * @note Blocks until lock is acquired
 */
eponMgr_data_t* eponMgr_data_lock(void)
{
    if (!g_eponData) return NULL;
    pthread_mutex_lock(&g_eponData->mutex);
    return g_eponData;
}

/**
 * @brief Unlock global data context after use
 * 
 * Releases the mutex acquired by eponMgr_data_lock(). Must be called
 * after finishing access to the locked data context.
 * 
 * @note Safe to call even if not initialized (no-op)
 * @note Must be called from the same thread that called lock
 */
void eponMgr_data_unlock(void)
{
    if (!g_eponData) return;
    pthread_mutex_unlock(&g_eponData->mutex);
}

/**
 * @brief Get EPON HAL API version
 * 
 * Returns the HAL API version for compatibility checking. Version format is
 * 0xMMmmpppp where MM=major, mm=minor, pppp=patch.
 * 
 * @return API version from epon_hal_get_version()
 * 
 * @note This is a direct passthrough to HAL API
 * @note No locking required - version is constant
 */
uint32_t eponMgr_data_get_hal_version(void)
{
    return epon_hal_get_version();
}

/**
 * @brief Initialize EPON HAL (cached in onu_state)
 * 
 * Calls epon_hal_init() with the configured callbacks and marks the HAL as
 * initialized in both data context and ONU state. This must be called before
 * any HAL operations.
 * 
 * @param eponData Pointer to data context
 * @return EPON HAL return code (EPON_HAL_SUCCESS or error code)
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Sets hal_initialized flag on success
 * @note HAL initialization status is cached in ONU state
 */
epon_hal_return_t eponMgr_data_hal_init(eponMgr_data_t *eponData)
{
    if (!eponData) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Initializing EPON HAL\n");
    
    pthread_mutex_lock(&eponData->mutex);
    EPONMGR_LOG_INFO("HAL configuration: status_cb=%p, interface_cb=%p, alarm_cb=%p, dpoe_supported=%d, config_size=%zu\n",
                         eponData->hal_config.status_callback,
                         eponData->hal_config.interface_status_callback,
                         eponData->hal_config.alarm_callback,
                         eponData->hal_config.dpoe_supported,
                         sizeof(epon_hal_config_t));

    epon_hal_return_t ret = epon_hal_init(&eponData->hal_config);
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

/**
 * @brief Get link statistics (with 30s cache) - zero-copy
 * 
 * Returns link statistics from cache if valid within TTL, otherwise calls HAL
 * to refresh the cache. Provides zero-copy access to cached statistics structure.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to stats if valid/cached, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Returned pointer valid until next cache invalidation
 * @note Cache TTL is configurable (default 30 seconds)
 */
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
    
    epon_hal_return_t ret = epon_hal_get_link_stats(ptr);
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

/**
 * @brief Get transceiver statistics (with 30s cache) - zero-copy
 * 
 * Returns transceiver statistics from cache if valid within TTL, otherwise calls
 * HAL to refresh the cache. Provides zero-copy access to cached statistics structure.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to stats if valid/cached, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Returned pointer valid until next cache invalidation
 * @note Cache TTL is configurable (default 30 seconds)
 */
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
    
    epon_hal_return_t ret = epon_hal_get_transceiver_stats(ptr);
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

/**
 * @brief Get LLID list - zero-copy
 * 
 * Calls HAL to populate the internal LLID list structure and returns a const pointer.
 * Automatically triggers TR-181 table sync if LLID count changes.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to LLID list if available, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Always calls HAL for fresh data (no cache for dynamic data)
 * @note Triggers TR-181 sync if count changes
 */
const epon_llid_list_t* eponMgr_data_get_llid_info(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    EPONMGR_LOG_DEBUG("Getting LLID information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Free old HAL-allocated memory before getting new data
    if (eponData->llid_list.llid_list) {
        free(eponData->llid_list.llid_list);
        eponData->llid_list.llid_list = NULL;
        eponData->llid_list.llid_count = 0;
    }
    
    // Call HAL to fill owned LLID list directly (zero-copy)
    // HAL will allocate new memory for llid_list
    epon_hal_return_t ret = epon_hal_get_llid_info(&eponData->llid_list);
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

/**
 * @brief Get interface list - zero-copy
 * 
 * Calls HAL to populate the internal interface list structure and returns a const
 * pointer. Automatically triggers TR-181 VEIP table sync if interface count changes.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to interface list if available, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Always calls HAL for fresh data (no cache for dynamic data)
 * @note Triggers TR-181 sync if count changes
 */
const epon_interface_list_t* eponMgr_data_get_interface_list(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    EPONMGR_LOG_INFO("Getting interface list\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Call HAL to fill owned interface list directly (zero-copy)
    epon_hal_return_t ret = epon_hal_get_interface_list(&eponData->interface_list);
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

/**
 * @brief Get OLT information (cached with validity flag) - zero-copy
 * 
 * Returns OLT information from cache if valid, otherwise calls HAL to refresh.
 * OLT information is cached because it rarely changes during ONU lifetime.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to OLT info if valid/cached, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Cache remains valid until explicit invalidation
 * @note Typically populated during ONU registration
 */
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
    
    epon_hal_return_t ret = epon_hal_get_olt_info(ptr);
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

/**
 * @brief Get ONU manufacturer information (cached with validity flag) - zero-copy
 * 
 * Returns ONU manufacturer information from cache if valid, otherwise calls HAL
 * to refresh. Manufacturer info is cached because it is constant for device lifetime.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to manufacturer info if valid/cached, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Cache remains valid until explicit invalidation
 * @note Contains vendor name, model, serial number, etc.
 */
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
    
    epon_hal_return_t ret = epon_hal_get_manufacturer_info(ptr);
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

/**
 * @brief Get link information (cached with validity flag) - zero-copy
 * 
 * Returns link information from cache if valid, otherwise calls HAL to refresh.
 * Link info includes mode, encryption status, and other link-level details.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to link info if valid/cached, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Cache remains valid until explicit invalidation
 * @note Includes link mode (1G/10G) and encryption status
 */
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
    
    epon_hal_return_t ret = epon_hal_get_link_info(ptr);
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

/**
 * @brief Get maximum CPE count
 * 
 * Returns the maximum number of CPE devices that can be supported by the ONU.
 * This value is returned from the CPE MAC table structure.
 * 
 * @param eponData Pointer to data context
 * @param max_cpe Pointer to store max CPE count
 * @return EPON_HAL_SUCCESS on success, EPON_HAL_ERROR_INVALID_PARAM on error
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Value is from CPE table structure
 */
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

/**
 * @brief Get CPE MAC address table - zero-copy
 * 
 * Calls DPoE HAL to populate the internal CPE table structure and returns a const
 * pointer. Automatically triggers TR-181 CPE table sync if count changes.
 * 
 * @param eponData Pointer to data context
 * @return Const pointer to CPE table if available, NULL otherwise
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Always calls HAL for fresh data (no cache for dynamic data)
 * @note Triggers TR-181 sync if total count changes
 * @note Total count = static_cpe_count + dynamic_cpe_count
 */
const dpoe_cpe_mac_table_t* eponMgr_data_get_cpe_mac_table(eponMgr_data_t *eponData)
{
    if (!eponData) return NULL;
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Free old HAL-allocated memory before getting new data
    if (eponData->cpe_table.cpe_list) {
        free(eponData->cpe_table.cpe_list);
        eponData->cpe_table.cpe_list = NULL;
        eponData->cpe_table.static_cpe_count = 0;
        eponData->cpe_table.dynamic_cpe_count = 0;
    }
    
    // Call HAL to fill owned CPE table directly (zero-copy)
    // HAL will allocate new memory for cpe_list
    epon_hal_return_t ret = dpoe_hal_get_cpe_mac_table(&eponData->cpe_table);
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

/**
 * @brief Set OAM log level (no caching)
 * 
 * Sets the OAM log level mask in the HAL for debugging purposes. This is a
 * direct passthrough to the HAL without caching.
 * 
 * @param eponData Pointer to data context
 * @param log_level Log level bitmask
 * @return EPON HAL return code
 * 
 * @note No caching - direct HAL call
 * @note Used for runtime debug level control
 */
epon_hal_return_t eponMgr_data_set_oam_log_level(eponMgr_data_t *eponData,
                                    uint32_t log_level)
{
    if (!eponData) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Setting OAM log level: 0x%08x\n", log_level);
    
    // No caching for log level settings
    return epon_hal_set_oam_log_mask(log_level);
}

/**
 * @brief Invalidate all cache entries (call on ONU status change)
 * 
 * Invalidates all cached statistics and ONU information, forcing fresh data
 * to be retrieved from HAL on next access. Also resets count caches to force
 * TR-181 table synchronization.
 * 
 * @param eponData Pointer to data context
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Should be called when ONU status changes significantly
 * @note Resets all count caches to trigger TR-181 sync
 */
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

/**
 * @brief Get LLID entry by index (for TR-181 access)
 * 
 * Returns a copy of the LLID entry at the specified index from the internal
 * list. Caller must populate data by calling eponMgr_data_get_llid_info() first.
 * 
 * @param eponData Pointer to data context
 * @param index Zero-based index into LLID list
 * @param llid_info Output LLID info structure (copy)
 * @return 0 on success, -1 on error or index out of range
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Caller must call eponMgr_data_get_llid_info() first to populate list
 * @note Returns a copy, not a pointer
 */
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

/**
 * @brief Get CPE entry by index (for TR-181 access)
 * 
 * Returns a copy of the CPE entry at the specified index from the internal
 * table. Caller must populate data by calling eponMgr_data_get_cpe_mac_table() first.
 * 
 * @param eponData Pointer to data context
 * @param index Zero-based index into CPE list
 * @param cpe_entry Output CPE entry structure (copy)
 * @return 0 on success, -1 on error or index out of range
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Caller must call eponMgr_data_get_cpe_mac_table() first to populate table
 * @note Returns a copy, not a pointer
 */
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

/**
 * @brief Get interface entry by index (for TR-181 access)
 * 
 * Returns a copy of the interface entry at the specified index from the internal
 * list. Caller must populate data by calling eponMgr_data_get_interface_list() first.
 * 
 * @param eponData Pointer to data context
 * @param index Zero-based index into interface list
 * @param if_info Output interface info structure (copy)
 * @return 0 on success, -1 on error or index out of range
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Caller must call eponMgr_data_get_interface_list() first to populate list
 * @note Returns a copy, not a pointer
 */
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

/**
 * @brief Get interface entry by name (for TR-181 access)
 * 
 * Searches the interface list for an interface with the specified name and
 * returns a copy. Caller must populate data by calling eponMgr_data_get_interface_list() first.
 * 
 * @param eponData Pointer to data context
 * @param name Interface name to search for (e.g., "veip0")
 * @param if_info Output interface info structure (copy)
 * @return 0 on success, -1 on error or not found
 * 
 * @note Thread-safe: Acquires and releases internal mutex
 * @note Caller must call eponMgr_data_get_interface_list() first to populate list
 * @note Returns a copy, not a pointer
 * @note Returns -1 if interface name not found
 */
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

/**
 * @brief Clear all statistics counters
 * 
 * Resets all statistics counters in the EPON data context and calls
 * the HAL clear_stats function if available.
 * 
 * @param eponData Pointer to data context
 */
void eponMgr_data_clear_stats(eponMgr_data_t *eponData)
{
    if (!eponData) return;
    
    pthread_mutex_lock(&eponData->mutex);
    
    /* Clear stats_data if allocated */
    if (eponData->stats_data) {
        memset(eponData->stats_data, 0, sizeof(eponMgr_statsData_t));
    }
    
    /* Clear link stats cache */
    memset(&eponData->stats_data->cached_link_stats, 0, sizeof(epon_hal_link_stats_t));
    memset(&eponData->stats_data->cached_transceiver_stats, 0, sizeof(epon_hal_transceiver_stats_t));
    
    pthread_mutex_unlock(&eponData->mutex);
    
    /* Call HAL clear stats function */
    if (epon_hal_clear_stats) {
        epon_hal_clear_stats();
    }
}
