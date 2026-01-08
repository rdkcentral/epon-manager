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
 * Zero-copy design: HAL fills .data directly, getters return const pointers
 */
typedef struct {
    pthread_mutex_t mutex;                           /**< Mutex for thread safety */
    uint32_t ttl_seconds;                            /**< Time-to-live for statistics */
    eponMgr_statsData_link_stats_t link_stats;      /**< TTL-based storage */
    eponMgr_statsData_transceiver_stats_t transceiver_stats; /**< TTL-based storage */
    eponMgr_statsData_manufacturer_info_t manufacturer_info; /**< Validity flag only */
    eponMgr_statsData_link_info_t link_info;        /**< Validity flag only */
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

/**
 * @brief Invalidate all data entries (called on ONU status change)
 * @param stats_data Pointer to stats data structure
 * 
 * This should be called whenever the EPON ONU status changes to ensure
 * all stored data is refreshed.
 */
void eponMgr_statsData_invalidate_all(eponMgr_statsData_t *stats_data);

/**
 * @brief Calculate Bit Error Rate (BER) from link statistics
 * @param link_stats Pointer to link statistics structure
 * @return BER as a double (e.g., 1e-9 for 1 error per billion bits)
 * 
 * BER = Total Bit Errors / Total Bits Received
 * 
 * Includes both corrected and uncorrectable errors:
 * - fec_corrected: number of corrected bit errors
 * - fec_uncorrectable: number of uncorrectable codewords (estimated as 8 errors each)
 * 
 * Returns 0.0 if no data received yet.
 */
double eponMgr_statsData_calculate_ber(const epon_hal_link_stats_t *link_stats);

#endif /* EPONMGR_STATSDATA_H */
