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

// Cache key prefixes
#define CACHE_KEY_LINK_STATS        "link_stats"
#define CACHE_KEY_TRANSCEIVER_STATS "transceiver_stats"

// Global EPON data context
static eponMgr_data_t *g_eponData = NULL;

int eponMgr_data_init(eponMgr_data_t *eponData, 
                      epon_hal_config_t *config, 
                      uint32_t cache_ttl)
{
    if (!eponData || !config) return -1;
    
    EPONMGR_LOG_INFO("Initializing core data context with cache TTL: %u seconds\n", cache_ttl);
    
    memset(eponData, 0, sizeof(eponMgr_data_t));
    
    // Initialize cache
    eponData->stats_data = (eponMgr_statsData_t *)malloc(sizeof(eponMgr_statsData_t));
    if (!eponData->stats_data) return -1;

    if (eponMgr_statsData_init(eponData->stats_data, cache_ttl) != 0) {
        free(eponData->stats_data);
        return -1;
    }
    
    // Initialize data structures
    eponData->interface_list = (eponMgr_interface_list_t *)malloc(sizeof(eponMgr_interface_list_t));
    if (!eponData->interface_list) goto error;
    if (eponMgr_interface_list_init(eponData->interface_list) != 0) goto error;
    
    eponData->llid_list = (eponMgr_llid_list_t *)malloc(sizeof(eponMgr_llid_list_t));
    if (!eponData->llid_list) goto error;
    if (eponMgr_llid_list_init(eponData->llid_list, 32) != 0) goto error;
    
    eponData->cpe_list = (eponMgr_cpe_list_t *)malloc(sizeof(eponMgr_cpe_list_t));
    if (!eponData->cpe_list) goto error;
    if (eponMgr_cpe_list_init(eponData->cpe_list, 256) != 0) goto error;
    
    eponData->onu_state = (eponMgr_onu_state_t *)malloc(sizeof(eponMgr_onu_state_t));
    if (!eponData->onu_state) goto error;
    if (eponMgr_onu_state_init(eponData->onu_state, false) != 0) goto error;
    
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

error:
    if (eponData->onu_state) {
        eponMgr_onu_state_destroy(eponData->onu_state);
        free(eponData->onu_state);
    }
    if (eponData->cpe_list) {
        eponMgr_cpe_list_destroy(eponData->cpe_list);
        free(eponData->cpe_list);
    }
    if (eponData->llid_list) {
        eponMgr_llid_list_destroy(eponData->llid_list);
        free(eponData->llid_list);
    }
    if (eponData->interface_list) {
        eponMgr_interface_list_destroy(eponData->interface_list);
        free(eponData->interface_list);
    }
    if (eponData->stats_data) {
        eponMgr_statsData_destroy(eponData->stats_data);
        free(eponData->stats_data);
    }
    return -1;
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
    if (eponData->cpe_list) {
        eponMgr_cpe_list_destroy(eponData->cpe_list);
        free(eponData->cpe_list);
    }
    if (eponData->llid_list) {
        eponMgr_llid_list_destroy(eponData->llid_list);
        free(eponData->llid_list);
    }
    if (eponData->interface_list) {
        eponMgr_interface_list_destroy(eponData->interface_list);
        free(eponData->interface_list);
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

int eponMgr_data_get_link_stats(eponMgr_data_t *eponData,
                                 epon_hal_link_stats_t *stats)
{
    if (!eponData || !stats) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting link statistics\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Try stored data first - returns true if cache is valid
    if (eponMgr_statsData_get_link_stats(eponData->stats_data, stats)) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_INFO("Link statistics retrieved from cache\n");
        return EPON_HAL_SUCCESS;  // Data available
    }
    
    // Data not available - call HAL
    memset(stats, 0, sizeof(epon_hal_link_stats_t));
    stats->struct_size = sizeof(epon_hal_link_stats_t);
    int ret = epon_hal_get_link_stats(stats);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_statsData_set_link_stats(eponData->stats_data, stats);
        EPONMGR_LOG_INFO("Link statistics retrieved from HAL and cached\n");
    } else {
        EPONMGR_LOG_INFO("Failed to get link statistics from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_transceiver_stats(eponMgr_data_t *eponData,
                                        epon_hal_transceiver_stats_t *stats)
{
    if (!eponData || !stats) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting transceiver statistics\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Try stored data first
    if (eponMgr_statsData_get_transceiver_stats(eponData->stats_data, stats)) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_INFO("Transceiver statistics retrieved from cache\n");
        return EPON_HAL_SUCCESS;
    }
    
    // Data not available - call HAL
    memset(stats, 0, sizeof(epon_hal_transceiver_stats_t));
    stats->struct_size = sizeof(epon_hal_transceiver_stats_t);
    int ret = epon_hal_get_transceiver_stats(stats);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_statsData_set_transceiver_stats(eponData->stats_data, stats);
        EPONMGR_LOG_INFO("Transceiver statistics retrieved from HAL and cached\n");
    } else {
        EPONMGR_LOG_INFO("Failed to get transceiver statistics from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_llid_info(eponMgr_data_t *eponData,
                                epon_llid_list_t *llid_list)
{
    if (!eponData || !llid_list) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting LLID information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Call HAL to get current LLID list
    int ret = epon_hal_get_llid_info(llid_list);
    if (ret == EPON_HAL_SUCCESS) {
        // Check if LLID list has changed
        uint32_t old_count = eponMgr_llid_list_count(eponData->llid_list);
        bool changed = (old_count != llid_list->llid_count);
        
        // Update internal LLID list data structure
        eponMgr_llid_list_clear(eponData->llid_list);
        
        for (uint32_t i = 0; i < llid_list->llid_count; i++) {
            int update_ret = eponMgr_llid_list_update(eponData->llid_list, &llid_list->llid_list[i]);
            if (update_ret == 1) {
                changed = true;  // New LLID added
            }
        }
        
        // Only sync TR-181 if LLID list changed
        if (changed) {
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("LLID list changed, updating TR-181\n");
            eponMgr_tr181_sync_llid_table();
            return ret;
        }
    } else {
        EPONMGR_LOG_INFO("Failed to get LLID information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_interface_list(eponMgr_data_t *eponData,
                                     epon_interface_list_t *if_list)
{
    if (!eponData || !if_list) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting interface list\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    /* Save old count for change detection */
    uint32_t old_count = eponMgr_interface_list_count(eponData->interface_list);
    
    // Call HAL to get current interface list
    int ret = epon_hal_get_interface_list(if_list);
    if (ret == EPON_HAL_SUCCESS) {
        EPONMGR_LOG_INFO("Retrieved %u interface(s) from HAL\n", if_list->interface_count);
        // Update internal interface list data structure
        eponMgr_interface_list_clear(eponData->interface_list);
        
        bool changed = (old_count != if_list->interface_count);
        
        for (uint32_t i = 0; i < if_list->interface_count; i++) {
            int update_ret = eponMgr_interface_list_update(eponData->interface_list, &if_list->interface[i]);
            if (update_ret == 1) {
                changed = true;  /* New interface added */
            }
        }
        
        /* Only sync TR-181 if interface list changed */
        if (changed) {
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("Interface list changed, updating TR-181\n");
            eponMgr_tr181_sync_veip_table();
            return ret;
        }
    } else {
        EPONMGR_LOG_INFO("Failed to get interface list from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_olt_info(eponMgr_data_t *eponData,
                               epon_olt_info_t *olt_info)
{
    if (!eponData || !olt_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting OLT information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_olt_info(eponData->onu_state, olt_info) == 0) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_INFO("OLT information retrieved from cache\n");
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    memset(olt_info, 0, sizeof(epon_olt_info_t));
    olt_info->struct_size = sizeof(epon_olt_info_t);
    int ret = epon_hal_get_olt_info(olt_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_olt_info(eponData->onu_state, olt_info);
        EPONMGR_LOG_INFO("OLT information retrieved from HAL and cached\n");
    } else {
        EPONMGR_LOG_INFO("Failed to get OLT information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_onu_manufacturer_info(eponMgr_data_t *eponData,
                                            epon_onu_manufacturer_info_t *mfr_info)
{
    if (!eponData || !mfr_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting ONU manufacturer information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_manufacturer_info(eponData->onu_state, mfr_info) == 0) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_INFO("ONU manufacturer information retrieved from cache\n");
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    memset(mfr_info, 0, sizeof(epon_onu_manufacturer_info_t));
    mfr_info->struct_size = sizeof(epon_onu_manufacturer_info_t);
    int ret = epon_hal_get_manufacturer_info(mfr_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_manufacturer_info(eponData->onu_state, mfr_info);
        EPONMGR_LOG_INFO("ONU manufacturer information retrieved from HAL and cached\n");
    } else {
        EPONMGR_LOG_INFO("Failed to get ONU manufacturer information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_link_info(eponMgr_data_t *eponData,
                                epon_hal_link_info_t *link_info)
{
    if (!eponData || !link_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting link information\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_link_info(eponData->onu_state, link_info) == 0) {
        pthread_mutex_unlock(&eponData->mutex);
        EPONMGR_LOG_INFO("Link information retrieved from cache\n");
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    memset(link_info, 0, sizeof(epon_hal_link_info_t));
    int ret = epon_hal_get_link_info(link_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_link_info(eponData->onu_state, link_info);
        EPONMGR_LOG_INFO("Link information retrieved from HAL and cached\n");
    } else {
        EPONMGR_LOG_INFO("Failed to get link information from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
}

int eponMgr_data_get_max_cpe(eponMgr_data_t *eponData,
                              uint32_t *max_cpe)
{
    if (!eponData || !max_cpe) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting maximum CPE count\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    // Return from internal CPE list structure
    *max_cpe = eponData->cpe_list->cpe_table.max_cpe;
    
    EPONMGR_LOG_INFO("Maximum CPE count: %u\n", *max_cpe);
    
    pthread_mutex_unlock(&eponData->mutex);
    return EPON_HAL_SUCCESS;
}

int eponMgr_data_get_cpe_mac_table(eponMgr_data_t *eponData,
                                    dpoe_cpe_mac_table_t *cpe_table)
{
    if (!eponData || !cpe_table) return EPON_HAL_ERROR_INVALID_PARAM;
    
    EPONMGR_LOG_INFO("Getting CPE MAC address table\n");
    
    pthread_mutex_lock(&eponData->mutex);
    
    /* Save old count for change detection */
    uint32_t old_count = eponMgr_cpe_list_count(eponData->cpe_list);
    
    /* Clear list before populating with fresh data */
    eponMgr_cpe_list_clear(eponData->cpe_list);
    
    // Call HAL to get current CPE table
    int ret = dpoe_hal_get_cpe_mac_table(cpe_table);
    if (ret == EPON_HAL_SUCCESS) {
        EPONMGR_LOG_INFO("Retrieved CPE MAC table: %u static, %u dynamic entries\n",
                        cpe_table->static_cpe_count, cpe_table->dynamic_cpe_count);
        // Update internal CPE list data structure
        bool has_new_cpes = false;
        uint32_t total = cpe_table->static_cpe_count + cpe_table->dynamic_cpe_count;
        
        for (uint32_t i = 0; i < total; i++) {
            int update_ret = eponMgr_cpe_list_update(eponData->cpe_list, &cpe_table->cpe_list[i]);
            if (update_ret == 1) {
                has_new_cpes = true;  /* New CPE added */
            }
        }
        
        /* Get new count */
        uint32_t new_count = eponMgr_cpe_list_count(eponData->cpe_list);
        
        /* Only sync if count changed or new CPEs added */
        if (new_count != old_count || has_new_cpes) {
            pthread_mutex_unlock(&eponData->mutex);
            EPONMGR_LOG_INFO("CPE table changed, updating TR-181\n");
            eponMgr_tr181_sync_cpe_table();
            return ret;
        }
    } else {
        EPONMGR_LOG_INFO("Failed to get CPE MAC table from HAL\n");
    }
    
    pthread_mutex_unlock(&eponData->mutex);
    return ret;
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
    
    pthread_mutex_unlock(&eponData->mutex);
}
