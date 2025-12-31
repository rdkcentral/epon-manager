/*
 * EPON Manager - HAL Wrapper Implementation
 * Wraps all EPON HAL APIs with cache management
 */

/* Include logger FIRST to enable HAL logging via HAL_LOG_FUNCTION */
#include "eponMgr_logger.h"

#include "eponMgr_hal_wrapper.h"
#include "eponMgr_tr181.h"
#include <string.h>
#include <stdlib.h>

// Cache key prefixes
#define CACHE_KEY_LINK_STATS        "link_stats"
#define CACHE_KEY_TRANSCEIVER_STATS "transceiver_stats"

int eponMgr_hal_wrapper_init(eponMgr_hal_wrapper_t *wrapper, 
                              epon_hal_config_t *config, 
                              uint32_t cache_ttl)
{
    if (!wrapper || !config) return -1;
    
    memset(wrapper, 0, sizeof(eponMgr_hal_wrapper_t));
    
    // Initialize cache
    wrapper->stats_cache = (eponMgr_cache_t *)malloc(sizeof(eponMgr_cache_t));
    if (!wrapper->stats_cache) return -1;
    
    if (eponMgr_cache_init(wrapper->stats_cache, cache_ttl) != 0) {
        free(wrapper->stats_cache);
        return -1;
    }
    
    // Initialize data structures
    wrapper->interface_list = (eponMgr_interface_list_t *)malloc(sizeof(eponMgr_interface_list_t));
    if (!wrapper->interface_list) goto error;
    if (eponMgr_interface_list_init(wrapper->interface_list) != 0) goto error;
    
    wrapper->llid_list = (eponMgr_llid_list_t *)malloc(sizeof(eponMgr_llid_list_t));
    if (!wrapper->llid_list) goto error;
    if (eponMgr_llid_list_init(wrapper->llid_list, 32) != 0) goto error;
    
    wrapper->cpe_list = (eponMgr_cpe_list_t *)malloc(sizeof(eponMgr_cpe_list_t));
    if (!wrapper->cpe_list) goto error;
    if (eponMgr_cpe_list_init(wrapper->cpe_list, 256) != 0) goto error;
    
    wrapper->onu_state = (eponMgr_onu_state_t *)malloc(sizeof(eponMgr_onu_state_t));
    if (!wrapper->onu_state) goto error;
    if (eponMgr_onu_state_init(wrapper->onu_state, false) != 0) goto error;
    
    // Store HAL config
    memcpy(&wrapper->hal_config, config, sizeof(epon_hal_config_t));
    
    pthread_mutex_init(&wrapper->mutex, NULL);
    wrapper->hal_initialized = false;
    
    return 0;

error:
    if (wrapper->onu_state) {
        eponMgr_onu_state_destroy(wrapper->onu_state);
        free(wrapper->onu_state);
    }
    if (wrapper->cpe_list) {
        eponMgr_cpe_list_destroy(wrapper->cpe_list);
        free(wrapper->cpe_list);
    }
    if (wrapper->llid_list) {
        eponMgr_llid_list_destroy(wrapper->llid_list);
        free(wrapper->llid_list);
    }
    if (wrapper->interface_list) {
        eponMgr_interface_list_destroy(wrapper->interface_list);
        free(wrapper->interface_list);
    }
    if (wrapper->stats_cache) {
        eponMgr_cache_destroy(wrapper->stats_cache);
        free(wrapper->stats_cache);
    }
    return -1;
}

void eponMgr_hal_wrapper_destroy(eponMgr_hal_wrapper_t *wrapper)
{
    if (!wrapper) return;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    if (wrapper->onu_state) {
        eponMgr_onu_state_destroy(wrapper->onu_state);
        free(wrapper->onu_state);
    }
    if (wrapper->cpe_list) {
        eponMgr_cpe_list_destroy(wrapper->cpe_list);
        free(wrapper->cpe_list);
    }
    if (wrapper->llid_list) {
        eponMgr_llid_list_destroy(wrapper->llid_list);
        free(wrapper->llid_list);
    }
    if (wrapper->interface_list) {
        eponMgr_interface_list_destroy(wrapper->interface_list);
        free(wrapper->interface_list);
    }
    if (wrapper->stats_cache) {
        eponMgr_cache_destroy(wrapper->stats_cache);
        free(wrapper->stats_cache);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    pthread_mutex_destroy(&wrapper->mutex);
}

uint32_t eponMgr_hal_wrapper_get_version(void)
{
    return epon_hal_get_version();
}

int eponMgr_hal_wrapper_hal_init(eponMgr_hal_wrapper_t *wrapper)
{
    if (!wrapper) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    int ret = epon_hal_init(&wrapper->hal_config);
    if (ret == EPON_HAL_SUCCESS) {
        wrapper->hal_initialized = true;
        eponMgr_onu_state_set_hal_initialized(wrapper->onu_state);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_link_stats(eponMgr_hal_wrapper_t *wrapper,
                                        epon_hal_link_stats_t *stats)
{
    if (!wrapper || !stats) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Try cache first
    if (eponMgr_cache_get_link_stats(wrapper->stats_cache, stats) == 0) {
        pthread_mutex_unlock(&wrapper->mutex);
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    int ret = epon_hal_get_link_stats(stats);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_cache_set_link_stats(wrapper->stats_cache, stats);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_transceiver_stats(eponMgr_hal_wrapper_t *wrapper,
                                                epon_hal_transceiver_stats_t *stats)
{
    if (!wrapper || !stats) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Try cache first
    if (eponMgr_cache_get_transceiver_stats(wrapper->stats_cache, stats)) {
        pthread_mutex_unlock(&wrapper->mutex);
        return EPON_HAL_SUCCESS;
    }
    
    // Cache miss - call HAL
    int ret = epon_hal_get_transceiver_stats(stats);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_cache_set_transceiver_stats(wrapper->stats_cache, stats);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_llid_info(eponMgr_hal_wrapper_t *wrapper,
                                       epon_llid_list_t *llid_list)
{
    if (!wrapper || !llid_list) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Call HAL to get current LLID list
    int ret = epon_hal_get_llid_info(llid_list);
    if (ret == EPON_HAL_SUCCESS) {
        // Check if LLID list has changed
        uint32_t old_count = eponMgr_llid_list_count(wrapper->llid_list);
        bool changed = (old_count != llid_list->llid_count);
        
        // Update internal LLID list data structure
        eponMgr_llid_list_clear(wrapper->llid_list);
        
        for (uint32_t i = 0; i < llid_list->llid_count; i++) {
            int update_ret = eponMgr_llid_list_update(wrapper->llid_list, &llid_list->llid_list[i]);
            if (update_ret == 1) {
                changed = true;  // New LLID added
            }
        }
        
        // Only sync TR-181 if LLID list changed
        if (changed) {
            pthread_mutex_unlock(&wrapper->mutex);
            eponMgr_tr181_sync_llid_table();
            return ret;
        }
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_interface_list(eponMgr_hal_wrapper_t *wrapper,
                                            epon_interface_list_t *if_list)
{
    if (!wrapper || !if_list) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Call HAL to get current interface list
    int ret = epon_hal_get_interface_list(if_list);
    if (ret == EPON_HAL_SUCCESS) {
        // Update internal interface list data structure
        eponMgr_interface_list_clear(wrapper->interface_list);
        
        for (uint32_t i = 0; i < if_list->interface_count; i++) {
            eponMgr_interface_list_update(wrapper->interface_list, &if_list->interface[i]);
        }
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_olt_info(eponMgr_hal_wrapper_t *wrapper,
                                      epon_olt_info_t *olt_info)
{
    if (!wrapper || !olt_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_olt_info(wrapper->onu_state, olt_info) == 0) {
        pthread_mutex_unlock(&wrapper->mutex);
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    int ret = epon_hal_get_olt_info(olt_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_olt_info(wrapper->onu_state, olt_info);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_onu_manufacturer_info(eponMgr_hal_wrapper_t *wrapper,
                                                   epon_onu_manufacturer_info_t *mfr_info)
{
    if (!wrapper || !mfr_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_manufacturer_info(wrapper->onu_state, mfr_info) == 0) {
        pthread_mutex_unlock(&wrapper->mutex);
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    int ret = epon_hal_get_manufacturer_info(mfr_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_manufacturer_info(wrapper->onu_state, mfr_info);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_link_info(eponMgr_hal_wrapper_t *wrapper,
                                       epon_hal_link_info_t *link_info)
{
    if (!wrapper || !link_info) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Try cached version first (validity flag, no TTL)
    if (eponMgr_onu_state_get_link_info(wrapper->onu_state, link_info) == 0) {
        pthread_mutex_unlock(&wrapper->mutex);
        return EPON_HAL_SUCCESS;  // Cache hit
    }
    
    // Cache miss - call HAL
    int ret = epon_hal_get_link_info(link_info);
    if (ret == EPON_HAL_SUCCESS) {
        eponMgr_onu_state_update_link_info(wrapper->onu_state, link_info);
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_get_max_cpe(eponMgr_hal_wrapper_t *wrapper,
                                     uint32_t *max_cpe)
{
    if (!wrapper || !max_cpe) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Return from internal CPE list structure
    *max_cpe = wrapper->cpe_list->cpe_table.max_cpe;
    
    pthread_mutex_unlock(&wrapper->mutex);
    return EPON_HAL_SUCCESS;
}

int eponMgr_hal_wrapper_get_cpe_mac_table(eponMgr_hal_wrapper_t *wrapper,
                                           dpoe_cpe_mac_table_t *cpe_table)
{
    if (!wrapper || !cpe_table) return EPON_HAL_ERROR_INVALID_PARAM;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    /* Save old count for change detection */
    uint32_t old_count = eponMgr_cpe_list_count(wrapper->cpe_list);
    
    /* Clear list before populating with fresh data */
    eponMgr_cpe_list_clear(wrapper->cpe_list);
    
    // Call HAL to get current CPE table
    int ret = dpoe_hal_get_cpe_mac_table(cpe_table);
    if (ret == EPON_HAL_SUCCESS) {
        // Update internal CPE list data structure
        bool has_new_cpes = false;
        uint32_t total = cpe_table->static_cpe_count + cpe_table->dynamic_cpe_count;
        
        for (uint32_t i = 0; i < total; i++) {
            int update_ret = eponMgr_cpe_list_update(wrapper->cpe_list, &cpe_table->cpe_list[i]);
            if (update_ret == 1) {
                has_new_cpes = true;  /* New CPE added */
            }
        }
        
        /* Get new count */
        uint32_t new_count = eponMgr_cpe_list_count(wrapper->cpe_list);
        
        /* Only sync if count changed or new CPEs added */
        if (new_count != old_count || has_new_cpes) {
            pthread_mutex_unlock(&wrapper->mutex);
            eponMgr_tr181_sync_cpe_table();
            return ret;
        }
    }
    
    pthread_mutex_unlock(&wrapper->mutex);
    return ret;
}

int eponMgr_hal_wrapper_set_oam_log_level(eponMgr_hal_wrapper_t *wrapper,
                                           uint32_t log_level)
{
    if (!wrapper) return EPON_HAL_ERROR_INVALID_PARAM;
    
    // No caching for log level settings
    return epon_hal_set_oam_log_mask(log_level);
}

void eponMgr_hal_wrapper_invalidate_cache(eponMgr_hal_wrapper_t *wrapper)
{
    if (!wrapper) return;
    
    pthread_mutex_lock(&wrapper->mutex);
    
    // Invalidate all cache entries (both statistics and info)
    eponMgr_cache_invalidate_all(wrapper->stats_cache);
    eponMgr_onu_state_invalidate_all(wrapper->onu_state);
    
    pthread_mutex_unlock(&wrapper->mutex);
}
