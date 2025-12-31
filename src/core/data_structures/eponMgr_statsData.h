/**
 * @file eponMgr_statsData.h
 * @brief EPON Manager statistics and info data storage with TTL and validity tracking
 * 
 * Two data storage strategies:
 * 1. Statistics (link_stats, transceiver_stats): TTL-based with timestamps
 * 2. Info (link_info, manufacturer_info): Validity flag, invalidated on ONU status change
 * 
 * All data entries invalidated when EPON ONU status changes.
 * Thread-safe with pthread_mutex protection.
 */

#ifndef EPONMGR_STATSDATA_H
#define EPONMGR_STATSDATA_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include "../../../include/epon_hal.h"

/**
 * @brief Storage entry for statistics (with TTL)
 */
typedef struct {
    epon_hal_link_stats_t data;
    time_t timestamp;
    bool valid;
} eponMgr_statsData_link_stats_t;

/**
 * @brief Storage entry for transceiver statistics (with TTL)
 */
typedef struct {
    epon_hal_transceiver_stats_t data;
    time_t timestamp;
    bool valid;
} eponMgr_statsData_transceiver_stats_t;

/**
 * @brief Storage entry for manufacturer info (validity flag only)
 */
typedef struct {
    epon_onu_manufacturer_info_t data;
    bool valid;
} eponMgr_statsData_manufacturer_info_t;

/**
 * @brief Storage entry for link info (validity flag only)
 */
typedef struct {
    epon_hal_link_info_t data;
    bool valid;
} eponMgr_statsData_link_info_t;

/**
 * @brief Main statistics data structure (thread-safe)
 */
typedef struct {
    pthread_mutex_t mutex;                          /**< Mutex for thread safety */
    uint32_t ttl_seconds;                           /**< Time-to-live for statistics */
    eponMgr_statsData_link_stats_t link_stats;          /**< TTL-based storage */
    eponMgr_statsData_transceiver_stats_t transceiver_stats; /**< TTL-based storage */
    eponMgr_statsData_manufacturer_info_t manufacturer_info;  /**< Validity flag only */
    eponMgr_statsData_link_info_t link_info;                  /**< Validity flag only */
} eponMgr_statsData_t;

/**
 * @brief Initialize stats data storage with mutex
 * @param stats_data Pointer to stats data structure
 * @param ttl_seconds TTL for statistics in seconds
 * @return 0 on success, -1 on error
 */
int eponMgr_statsData_init(eponMgr_statsData_t *stats_data, uint32_t ttl_seconds);

/**
 * @brief Destroy stats data storage and cleanup mutex
 * @param stats_data Pointer to stats data structure
 */
void eponMgr_statsData_destroy(eponMgr_statsData_t *stats_data);

/**
 * @brief Check if statistics entry is valid (not expired)
 * @param timestamp Entry timestamp
 * @param ttl_seconds TTL in seconds
 * @return true if valid (not expired), false otherwise
 */
bool eponMgr_statsData_is_stats_valid(time_t timestamp, uint32_t ttl_seconds);

/* Statistics storage - TTL based */

/**
 * @brief Store link stats with timestamp
 * @param stats_data Pointer to stats data structure
 * @param stats Link statistics data
 */
void eponMgr_statsData_set_link_stats(eponMgr_statsData_t *stats_data, const epon_hal_link_stats_t *stats);

/**
 * @brief Get link stats (checks TTL)
 * @param stats_data Pointer to stats data structure
 * @param stats Output buffer for link statistics
 * @return true if data available (valid & not expired), false otherwise
 */
bool eponMgr_statsData_get_link_stats(eponMgr_statsData_t *stats_data, epon_hal_link_stats_t *stats);

/**
 * @brief Store transceiver stats with timestamp
 * @param stats_data Pointer to stats data structure
 * @param stats Transceiver statistics data
 */
void eponMgr_statsData_set_transceiver_stats(eponMgr_statsData_t *stats_data, const epon_hal_transceiver_stats_t *stats);

/**
 * @brief Get transceiver stats (checks TTL)
 * @param stats_data Pointer to stats data structure
 * @param stats Output buffer for transceiver statistics
 * @return true if data available (valid & not expired), false otherwise
 */
bool eponMgr_statsData_get_transceiver_stats(eponMgr_statsData_t *stats_data, epon_hal_transceiver_stats_t *stats);

/* Info storage - validity flag based (no timestamp) */

/**
 * @brief Store manufacturer info
 * @param stats_data Pointer to stats data structure
 * @param info Manufacturer information data
 */
void eponMgr_statsData_set_manufacturer_info(eponMgr_statsData_t *stats_data, const epon_onu_manufacturer_info_t *info);

/**
 * @brief Get manufacturer info (checks validity flag only)
 * @param stats_data Pointer to stats data structure
 * @param info Output buffer for manufacturer information
 * @return true if valid, false if invalid
 */
bool eponMgr_statsData_get_manufacturer_info(eponMgr_statsData_t *stats_data, epon_onu_manufacturer_info_t *info);

/**
 * @brief Store link info
 * @param stats_data Pointer to stats data structure
 * @param info Link information data
 */
void eponMgr_statsData_set_link_info(eponMgr_statsData_t *stats_data, const epon_hal_link_info_t *info);

/**
 * @brief Get link info (checks validity flag only)
 * @param stats_data Pointer to stats data structure
 * @param info Output buffer for link information
 * @return true if valid, false if invalid
 */
bool eponMgr_statsData_get_link_info(eponMgr_statsData_t *stats_data, epon_hal_link_info_t *info);

/**
 * @brief Invalidate all data entries (called on ONU status change)
 * @param stats_data Pointer to stats data structure
 * 
 * This should be called whenever the EPON ONU status changes to ensure
 * all stored data is refreshed.
 */
void eponMgr_statsData_invalidate_all(eponMgr_statsData_t *stats_data);

/**
 * @brief Invalidate specific data entry
 * @param stats_data Pointer to stats data structure
 * @param entry_name Name of entry to invalidate ("link_stats", "transceiver_stats", etc.)
 */
void eponMgr_statsData_invalidate(eponMgr_statsData_t *stats_data, const char *entry_name);

#endif /* EPONMGR_STATSDATA_H */
