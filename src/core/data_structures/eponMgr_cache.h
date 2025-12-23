/**
 * @file eponMgr_cache.h
 * @brief EPON Manager cache for HAL data
 * 
 * Two cache strategies:
 * 1. Statistics (link_stats, transceiver_stats): TTL-based with timestamps
 * 2. Info (link_info, manufacturer_info): Validity flag, invalidated on ONU status change
 * 
 * All cache entries invalidated when EPON ONU status changes.
 * Thread-safe with pthread_mutex protection.
 */

#ifndef EPONMGR_CACHE_H
#define EPONMGR_CACHE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include "../../../include/epon_hal.h"

/**
 * @brief Cache entry for statistics (with TTL)
 */
typedef struct {
    epon_hal_link_stats_t data;
    time_t timestamp;
    bool valid;
} eponMgr_cache_link_stats_t;

/**
 * @brief Cache entry for transceiver statistics (with TTL)
 */
typedef struct {
    epon_hal_transceiver_stats_t data;
    time_t timestamp;
    bool valid;
} eponMgr_cache_transceiver_stats_t;

/**
 * @brief Cache entry for manufacturer info (validity flag only)
 */
typedef struct {
    epon_onu_manufacturer_info_t data;
    bool valid;
} eponMgr_cache_manufacturer_info_t;

/**
 * @brief Cache entry for link info (validity flag only)
 */
typedef struct {
    epon_hal_link_info_t data;
    bool valid;
} eponMgr_cache_link_info_t;

/**
 * @brief Main cache structure (thread-safe)
 */
typedef struct {
    pthread_mutex_t mutex;                          /**< Mutex for thread safety */
    uint32_t ttl_seconds;                           /**< Time-to-live for statistics */
    eponMgr_cache_link_stats_t link_stats;          /**< TTL-based cache */
    eponMgr_cache_transceiver_stats_t transceiver_stats; /**< TTL-based cache */
    eponMgr_cache_manufacturer_info_t manufacturer_info;  /**< Validity flag only */
    eponMgr_cache_link_info_t link_info;                  /**< Validity flag only */
} eponMgr_cache_t;

/**
 * @brief Initialize cache with mutex
 * @param cache Pointer to cache structure
 * @param ttl_seconds Cache TTL for statistics in seconds
 * @return 0 on success, -1 on error
 */
int eponMgr_cache_init(eponMgr_cache_t *cache, uint32_t ttl_seconds);

/**
 * @brief Destroy cache and cleanup mutex
 * @param cache Pointer to cache structure
 */
void eponMgr_cache_destroy(eponMgr_cache_t *cache);

/**
 * @brief Check if statistics cache entry is valid (not expired)
 * @param timestamp Entry timestamp
 * @param ttl_seconds TTL in seconds
 * @return true if valid (not expired), false otherwise
 */
bool eponMgr_cache_is_stats_valid(time_t timestamp, uint32_t ttl_seconds);

/* Statistics cache - TTL based */

/**
 * @brief Store link stats in cache with timestamp
 * @param cache Pointer to cache structure
 * @param stats Link statistics data
 */
void eponMgr_cache_set_link_stats(eponMgr_cache_t *cache, const epon_hal_link_stats_t *stats);

/**
 * @brief Get link stats from cache (checks TTL)
 * @param cache Pointer to cache structure
 * @param stats Output buffer for link statistics
 * @return true if cache hit (valid & not expired), false if cache miss
 */
bool eponMgr_cache_get_link_stats(eponMgr_cache_t *cache, epon_hal_link_stats_t *stats);

/**
 * @brief Store transceiver stats in cache with timestamp
 * @param cache Pointer to cache structure
 * @param stats Transceiver statistics data
 */
void eponMgr_cache_set_transceiver_stats(eponMgr_cache_t *cache, const epon_hal_transceiver_stats_t *stats);

/**
 * @brief Get transceiver stats from cache (checks TTL)
 * @param cache Pointer to cache structure
 * @param stats Output buffer for transceiver statistics
 * @return true if cache hit (valid & not expired), false if cache miss
 */
bool eponMgr_cache_get_transceiver_stats(eponMgr_cache_t *cache, epon_hal_transceiver_stats_t *stats);

/* Info cache - validity flag based (no timestamp) */

/**
 * @brief Store manufacturer info in cache
 * @param cache Pointer to cache structure
 * @param info Manufacturer information data
 */
void eponMgr_cache_set_manufacturer_info(eponMgr_cache_t *cache, const epon_onu_manufacturer_info_t *info);

/**
 * @brief Get manufacturer info from cache (checks validity flag only)
 * @param cache Pointer to cache structure
 * @param info Output buffer for manufacturer information
 * @return true if valid, false if invalid
 */
bool eponMgr_cache_get_manufacturer_info(eponMgr_cache_t *cache, epon_onu_manufacturer_info_t *info);

/**
 * @brief Store link info in cache
 * @param cache Pointer to cache structure
 * @param info Link information data
 */
void eponMgr_cache_set_link_info(eponMgr_cache_t *cache, const epon_hal_link_info_t *info);

/**
 * @brief Get link info from cache (checks validity flag only)
 * @param cache Pointer to cache structure
 * @param info Output buffer for link information
 * @return true if valid, false if invalid
 */
bool eponMgr_cache_get_link_info(eponMgr_cache_t *cache, epon_hal_link_info_t *info);

/**
 * @brief Invalidate all cache entries (called on ONU status change)
 * @param cache Pointer to cache structure
 * 
 * This should be called whenever the EPON ONU status changes to ensure
 * all cached data is refreshed.
 */
void eponMgr_cache_invalidate_all(eponMgr_cache_t *cache);

/**
 * @brief Invalidate specific cache entry
 * @param cache Pointer to cache structure
 * @param entry_name Name of entry to invalidate ("link_stats", "transceiver_stats", etc.)
 */
void eponMgr_cache_invalidate(eponMgr_cache_t *cache, const char *entry_name);

#endif /* EPONMGR_CACHE_H */
