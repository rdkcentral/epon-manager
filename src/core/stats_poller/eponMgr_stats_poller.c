/**
 * @file eponMgr_stats_poller.c
 * @brief EPON Manager Statistics Polling Thread Implementation
 */

#include "eponMgr_stats_poller.h"
#include "eponMgr_logger.h"
#include "epon_hal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**
 * @brief Collect and update all statistics
 * @param poller Pointer to stats poller structure
 * @return 0 on success, -1 on error
 */
static int collect_all_stats(eponMgr_stats_poller_t *poller) {
    if (!poller || !poller->eponData) {
        return -1;
    }
    
    int errors = 0;
    
    /* Collect link statistics */
    epon_hal_link_stats_t link_stats;
    if (eponMgr_eponData_get_link_stats(poller->eponData, &link_stats) == EPON_HAL_SUCCESS) {
        EPONMGR_LOG_DEBUG("Stats poller: Collected link stats (TX: %llu bytes, RX: %llu bytes)\n",
                         (unsigned long long)link_stats.bytes_sent,
                         (unsigned long long)link_stats.bytes_received);
        
        // TODO: Push to telemetry system when implemented
        // eponMgr_telemetry_report_link_stats(&link_stats);
    } else {
        EPONMGR_LOG_WARN("Stats poller: Failed to collect link stats\n");
        errors++;
    }
    
    /* Collect transceiver statistics */
    epon_hal_transceiver_stats_t transceiver_stats;
    if (eponMgr_eponData_get_transceiver_stats(poller->eponData, &transceiver_stats) == EPON_HAL_SUCCESS) {
        EPONMGR_LOG_DEBUG("Stats poller: Collected transceiver stats (RX power: %.2f dBm, TX power: %.2f dBm)\n",
                         transceiver_stats.optical_signal_level,
                         transceiver_stats.transmit_optical_level);
        
        // TODO: Push to telemetry system when implemented
        // eponMgr_telemetry_report_transceiver_stats(&transceiver_stats);
    } else {
        EPONMGR_LOG_WARN("Stats poller: Failed to collect transceiver stats\n");
        errors++;
    }
    
    return (errors > 0) ? -1 : 0;
}

/**
 * @brief Stats poller thread main function
 * @param arg Pointer to eponMgr_stats_poller_t structure
 * @return NULL
 */
static void* stats_poller_thread(void *arg) {
    eponMgr_stats_poller_t *poller = (eponMgr_stats_poller_t *)arg;
    
    EPONMGR_LOG_INFO("Stats poller thread started (interval: %u seconds)\n", 
                     poller->interval_seconds);
    
    pthread_mutex_lock(&poller->mutex);
    poller->running = true;
    pthread_mutex_unlock(&poller->mutex);
    
    while (1) {
        pthread_mutex_lock(&poller->mutex);
        
        /* Check for shutdown */
        if (poller->shutdown_requested) {
            pthread_mutex_unlock(&poller->mutex);
            break;
        }
        
        /* Check if enabled */
        bool enabled = poller->enabled;
        uint32_t interval = poller->interval_seconds;
        
        pthread_mutex_unlock(&poller->mutex);
        
        if (enabled) {
            /* Collect statistics */
            if (collect_all_stats(poller) == 0) {
                EPONMGR_LOG_INFO("Stats poller: Successfully collected all statistics\n");
            } else {
                EPONMGR_LOG_WARN("Stats poller: Some statistics collection failed\n");
            }
        } else {
            EPONMGR_LOG_DEBUG("Stats poller: Skipping collection (disabled)\n");
        }
        
        /* Sleep for interval with condition variable for early wake */
        pthread_mutex_lock(&poller->mutex);
        
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += interval;
        
        /* Wait with timeout, can be woken up early by trigger_now or stop */
        int rc = pthread_cond_timedwait(&poller->cond, &poller->mutex, &ts);
        
        /* Check shutdown again after wake */
        if (poller->shutdown_requested) {
            pthread_mutex_unlock(&poller->mutex);
            break;
        }
        
        if (rc == 0) {
            /* Woken up early - either triggered or shutdown */
            EPONMGR_LOG_DEBUG("Stats poller: Woken up early\n");
        }
        
        pthread_mutex_unlock(&poller->mutex);
    }
    
    pthread_mutex_lock(&poller->mutex);
    poller->running = false;
    pthread_mutex_unlock(&poller->mutex);
    
    EPONMGR_LOG_INFO("Stats poller thread stopped\n");
    return NULL;
}

int eponMgr_stats_poller_init(eponMgr_stats_poller_t *poller,
                               eponMgr_data_t *eponData,
                               bool enabled,
                               uint32_t interval_seconds) {
    if (!poller || !eponData) {
        EPONMGR_LOG_ERROR("Invalid arguments to stats_poller_init\n");
        return -1;
    }
    
    memset(poller, 0, sizeof(eponMgr_stats_poller_t));
    
    poller->eponData = eponData;
    poller->enabled = enabled;
    poller->interval_seconds = interval_seconds;
    poller->running = false;
    poller->shutdown_requested = false;
    
    if (pthread_mutex_init(&poller->mutex, NULL) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize stats poller mutex\n");
        return -1;
    }
    
    if (pthread_cond_init(&poller->cond, NULL) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize stats poller condition variable\n");
        pthread_mutex_destroy(&poller->mutex);
        return -1;
    }
    
    EPONMGR_LOG_INFO("Stats poller initialized (enabled: %s, interval: %u seconds)\n",
                     enabled ? "true" : "false", interval_seconds);
    
    return 0;
}

int eponMgr_stats_poller_start(eponMgr_stats_poller_t *poller) {
    if (!poller) {
        return -1;
    }
    
    pthread_mutex_lock(&poller->mutex);
    
    if (poller->running) {
        pthread_mutex_unlock(&poller->mutex);
        EPONMGR_LOG_WARN("Stats poller already running\n");
        return -1;
    }
    
    poller->shutdown_requested = false;
    pthread_mutex_unlock(&poller->mutex);
    
    /* Create poller thread */
    if (pthread_create(&poller->thread, NULL, stats_poller_thread, poller) != 0) {
        EPONMGR_LOG_ERROR("Failed to create stats poller thread\n");
        return -1;
    }
    
    EPONMGR_LOG_INFO("Stats poller thread created\n");
    return 0;
}

void eponMgr_stats_poller_stop(eponMgr_stats_poller_t *poller) {
    if (!poller) return;
    
    pthread_mutex_lock(&poller->mutex);
    
    if (!poller->running) {
        pthread_mutex_unlock(&poller->mutex);
        return;
    }
    
    poller->shutdown_requested = true;
    pthread_cond_signal(&poller->cond);  /* Wake up thread */
    pthread_mutex_unlock(&poller->mutex);
    
    EPONMGR_LOG_INFO("Waiting for stats poller thread to stop...\n");
    
    /* Wait for thread to finish */
    pthread_join(poller->thread, NULL);
    
    EPONMGR_LOG_INFO("Stats poller thread joined\n");
}

void eponMgr_stats_poller_destroy(eponMgr_stats_poller_t *poller) {
    if (!poller) return;
    
    /* Ensure thread is stopped */
    eponMgr_stats_poller_stop(poller);
    
    /* Cleanup synchronization primitives */
    pthread_cond_destroy(&poller->cond);
    pthread_mutex_destroy(&poller->mutex);
    
    EPONMGR_LOG_INFO("Stats poller destroyed\n");
}

bool eponMgr_stats_poller_is_running(const eponMgr_stats_poller_t *poller) {
    if (!poller) return false;
    
    bool running;
    pthread_mutex_lock((pthread_mutex_t *)&poller->mutex);
    running = poller->running;
    pthread_mutex_unlock((pthread_mutex_t *)&poller->mutex);
    
    return running;
}

int eponMgr_stats_poller_set_enabled(eponMgr_stats_poller_t *poller, bool enabled) {
    if (!poller) return -1;
    
    pthread_mutex_lock(&poller->mutex);
    poller->enabled = enabled;
    pthread_mutex_unlock(&poller->mutex);
    
    EPONMGR_LOG_INFO("Stats poller %s\n", enabled ? "enabled" : "disabled");
    
    return 0;
}

int eponMgr_stats_poller_set_interval(eponMgr_stats_poller_t *poller, uint32_t interval_seconds) {
    if (!poller || interval_seconds == 0) return -1;
    
    pthread_mutex_lock(&poller->mutex);
    poller->interval_seconds = interval_seconds;
    pthread_cond_signal(&poller->cond);  /* Wake up to apply new interval */
    pthread_mutex_unlock(&poller->mutex);
    
    EPONMGR_LOG_INFO("Stats poller interval updated to %u seconds\n", interval_seconds);
    
    return 0;
}

int eponMgr_stats_poller_trigger_now(eponMgr_stats_poller_t *poller) {
    if (!poller) return -1;
    
    pthread_mutex_lock(&poller->mutex);
    
    if (!poller->running) {
        pthread_mutex_unlock(&poller->mutex);
        EPONMGR_LOG_WARN("Cannot trigger stats collection: poller not running\n");
        return -1;
    }
    
    /* Signal thread to wake up and collect stats immediately */
    pthread_cond_signal(&poller->cond);
    pthread_mutex_unlock(&poller->mutex);
    
    EPONMGR_LOG_INFO("Stats poller triggered for immediate collection\n");
    
    return 0;
}
