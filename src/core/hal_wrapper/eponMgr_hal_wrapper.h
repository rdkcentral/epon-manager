/*
 * EPON Manager - HAL Wrapper with Caching
 * Wraps all EPON HAL APIs with cache management
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

#ifndef EPONMGR_HAL_WRAPPER_H
#define EPONMGR_HAL_WRAPPER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "epon_hal.h"
#include "eponMgr_cache.h"
#include "eponMgr_interface_list.h"
#include "eponMgr_llid_list.h"
#include "eponMgr_cpe_list.h"
#include "eponMgr_onu_state.h"

/**
 * @brief HAL wrapper context with cache and data structures
 */
typedef struct {
    // Statistics cache (with TTL)
    eponMgr_cache_t *stats_cache;
    
    // Data structures for state management
    eponMgr_interface_list_t *interface_list;
    eponMgr_llid_list_t *llid_list;
    eponMgr_cpe_list_t *cpe_list;
    eponMgr_onu_state_t *onu_state;
    
    // HAL initialization state
    bool hal_initialized;
    epon_hal_config_t hal_config;
    
    // Thread safety
    pthread_mutex_t mutex;
} eponMgr_hal_wrapper_t;

/**
 * @brief Initialize HAL wrapper
 * @param wrapper Pointer to wrapper context
 * @param config HAL configuration with callbacks
 * @param cache_ttl Cache TTL in seconds (default 30)
 * @return 0 on success, -1 on error
 */
int eponMgr_hal_wrapper_init(eponMgr_hal_wrapper_t *wrapper, 
                              epon_hal_config_t *config, 
                              uint32_t cache_ttl);

/**
 * @brief Destroy HAL wrapper and cleanup resources
 * @param wrapper Pointer to wrapper context
 */
void eponMgr_hal_wrapper_destroy(eponMgr_hal_wrapper_t *wrapper);

/**
 * @brief Get EPON HAL API version
 * @return API version
 */
uint32_t eponMgr_hal_wrapper_get_version(void);

/**
 * @brief Initialize EPON HAL (cached in onu_state)
 * @param wrapper Pointer to wrapper context
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_hal_init(eponMgr_hal_wrapper_t *wrapper);

/**
 * @brief Get link statistics (with 30s cache)
 * @param wrapper Pointer to wrapper context
 * @param stats Pointer to stats structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_link_stats(eponMgr_hal_wrapper_t *wrapper,
                                        epon_hal_link_stats_t *stats);

/**
 * @brief Get transceiver statistics (with 30s cache)
 * @param wrapper Pointer to wrapper context
 * @param stats Pointer to stats structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_transceiver_stats(eponMgr_hal_wrapper_t *wrapper,
                                                epon_hal_transceiver_stats_t *stats);

/**
 * @brief Get LLID list (updates llid_list data structure)
 * @param wrapper Pointer to wrapper context
 * @param llid_list Pointer to LLID list structure (HAL allocates, caller frees)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_llid_info(eponMgr_hal_wrapper_t *wrapper,
                                       epon_llid_list_t *llid_list);

/**
 * @brief Get interface list (updates interface_list data structure)
 * @param wrapper Pointer to wrapper context
 * @param if_list Pointer to interface list structure
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_interface_list(eponMgr_hal_wrapper_t *wrapper,
                                            epon_interface_list_t *if_list);

/**
 * @brief Get OLT information (cached with validity flag)
 * @param wrapper Pointer to wrapper context
 * @param olt_info Pointer to OLT info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_olt_info(eponMgr_hal_wrapper_t *wrapper,
                                      epon_olt_info_t *olt_info);

/**
 * @brief Get ONU manufacturer information (cached with validity flag)
 * @param wrapper Pointer to wrapper context
 * @param mfr_info Pointer to manufacturer info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_onu_manufacturer_info(eponMgr_hal_wrapper_t *wrapper,
                                                   epon_onu_manufacturer_info_t *mfr_info);

/**
 * @brief Get link information (cached with validity flag)
 * @param wrapper Pointer to wrapper context
 * @param link_info Pointer to link info structure (caller sets struct_size)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_link_info(eponMgr_hal_wrapper_t *wrapper,
                                       epon_hal_link_info_t *link_info);

/**
 * @brief Get maximum CPE count
 * @param wrapper Pointer to wrapper context
 * @param max_cpe Pointer to max CPE count
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_max_cpe(eponMgr_hal_wrapper_t *wrapper,
                                     uint32_t *max_cpe);

/**
 * @brief Get CPE MAC address table (updates cpe_list data structure)
 * @param wrapper Pointer to wrapper context
 * @param cpe_table Pointer to CPE table (HAL allocates, caller frees)
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_get_cpe_mac_table(eponMgr_hal_wrapper_t *wrapper,
                                           dpoe_cpe_mac_table_t *cpe_table);

/**
 * @brief Set OAM log level (no caching)
 * @param wrapper Pointer to wrapper context
 * @param log_level Log level bitmask
 * @return EPON HAL return code
 */
int eponMgr_hal_wrapper_set_oam_log_level(eponMgr_hal_wrapper_t *wrapper,
                                           uint32_t log_level);

/**
 * @brief Invalidate all cache entries (call on ONU status change)
 * @param wrapper Pointer to wrapper context
 */
void eponMgr_hal_wrapper_invalidate_cache(eponMgr_hal_wrapper_t *wrapper);

#endif /* EPONMGR_HAL_WRAPPER_H */
