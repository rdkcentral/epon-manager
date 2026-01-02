/**
 * @file eponMgr_stats_poller.h
 * @brief EPON Manager Statistics Polling Thread (Harvester)
 * 
 * Optional periodic thread that polls statistics from HAL and updates:
 * - Internal cache with fresh statistics
 * - Telemetry system with periodic metrics
 * 
 * Controlled via PSM configuration:
 * - dmsb.eponmanager.StatsPollerEnabled (bool, default: false)
 * - dmsb.eponmanager.StatsPollerInterval (uint32, default: 900 seconds / 15 minutes)
 */

#ifndef EPONMGR_STATS_POLLER_H
#define EPONMGR_STATS_POLLER_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "../data_structures/eponMgr_data.h"

/**
 * @brief Stats poller context structure
 */
typedef struct {
    bool enabled;                        /**< Poller enabled/disabled */
    uint32_t interval_seconds;           /**< Polling interval in seconds */
    bool running;                        /**< Thread running flag */
    bool shutdown_requested;             /**< Shutdown signal */
    pthread_t thread;                    /**< Poller thread handle */
    pthread_mutex_t mutex;               /**< Mutex for thread control */
    pthread_cond_t cond;                 /**< Condition variable for wake/sleep */
    eponMgr_data_t *eponData;            /**< EPON data context for stats queries */
} eponMgr_stats_poller_t;

/**
 * @brief Initialize stats poller
 * @param poller Pointer to stats poller structure
 * @param eponData Pointer to EPON data context for stats queries
 * @param enabled Enable/disable polling
 * @param interval_seconds Polling interval in seconds
 * @return 0 on success, -1 on error
 */
int eponMgr_stats_poller_init(eponMgr_stats_poller_t *poller,
                               eponMgr_data_t *eponData,
                               bool enabled,
                               uint32_t interval_seconds);

/**
 * @brief Start stats polling thread
 * @param poller Pointer to stats poller structure
 * @return 0 on success, -1 on error
 */
int eponMgr_stats_poller_start(eponMgr_stats_poller_t *poller);

/**
 * @brief Stop stats polling thread
 * @param poller Pointer to stats poller structure
 */
void eponMgr_stats_poller_stop(eponMgr_stats_poller_t *poller);

/**
 * @brief Destroy stats poller and cleanup resources
 * @param poller Pointer to stats poller structure
 */
void eponMgr_stats_poller_destroy(eponMgr_stats_poller_t *poller);

/**
 * @brief Check if stats poller is running
 * @param poller Pointer to stats poller structure
 * @return true if running, false otherwise
 */
bool eponMgr_stats_poller_is_running(const eponMgr_stats_poller_t *poller);

/**
 * @brief Enable/disable stats poller at runtime
 * @param poller Pointer to stats poller structure
 * @param enabled true to enable, false to disable
 * @return 0 on success, -1 on error
 */
int eponMgr_stats_poller_set_enabled(eponMgr_stats_poller_t *poller, bool enabled);

/**
 * @brief Update polling interval at runtime
 * @param poller Pointer to stats poller structure
 * @param interval_seconds New interval in seconds
 * @return 0 on success, -1 on error
 */
int eponMgr_stats_poller_set_interval(eponMgr_stats_poller_t *poller, uint32_t interval_seconds);

/**
 * @brief Trigger immediate stats collection (bypasses interval timer)
 * @param poller Pointer to stats poller structure
 * @return 0 on success, -1 on error
 */
int eponMgr_stats_poller_trigger_now(eponMgr_stats_poller_t *poller);

#endif /* EPONMGR_STATS_POLLER_H */
