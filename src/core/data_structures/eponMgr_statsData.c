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
 * @file eponMgr_statsData.c
 * @brief EPON Manager statistics data storage with thread-safe mutex protection
 * 
 * Two data storage strategies:
 * 1. Statistics: TTL-based with timestamps
 * 2. Info data: Validity flag only (no expiration, invalidated on ONU status change)
 */

#include "eponMgr_statsData.h"
#include "eponMgr_logger.h"
#include <string.h>

int eponMgr_statsData_init(eponMgr_statsData_t *stats_data, uint32_t ttl_seconds) {
    if (!stats_data) return -1;
    
    EPONMGR_LOG_INFO("Initializing stats data storage with TTL: %u seconds\n", ttl_seconds);
    
    memset(stats_data, 0, sizeof(eponMgr_statsData_t));
    stats_data->ttl_seconds = ttl_seconds;
    
    if (pthread_mutex_init(&stats_data->mutex, NULL) != 0) {
        return -1;
    }
    
    return 0;
}

void eponMgr_statsData_destroy(eponMgr_statsData_t *stats_data) {
    if (!stats_data) return;
    
    EPONMGR_LOG_INFO("Destroying stats data storage\n");
    
    pthread_mutex_destroy(&stats_data->mutex);
}

bool eponMgr_statsData_is_stats_valid(time_t timestamp, uint32_t ttl_seconds) {
    if (timestamp == 0) {
        return false; /* Never cached */
    }
    
    time_t now = time(NULL);
    time_t age = now - timestamp;
    
    return (age >= 0 && age < (time_t)ttl_seconds);
}

void eponMgr_statsData_invalidate_all(eponMgr_statsData_t *stats_data) {
    if (!stats_data) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    /* Invalidate statistics storage */
    stats_data->link_stats.valid = false;
    stats_data->link_stats.timestamp = 0;
    
    stats_data->transceiver_stats.valid = false;
    stats_data->transceiver_stats.timestamp = 0;
    
    /* Invalidate info storage */
    stats_data->manufacturer_info.valid = false;
    stats_data->link_info.valid = false;
    
    pthread_mutex_unlock(&stats_data->mutex);
}

double eponMgr_statsData_calculate_ber(const epon_hal_link_stats_t *link_stats) {
    if (!link_stats || link_stats->bytes_received == 0) {
        return 0.0;
    }
    
    /* BER = Total Bit Errors / Total Bits Received
     * 
     * Total Bit Errors = fec_corrected + (fec_uncorrectable * estimated_errors_per_codeword)
     * 
     * - fec_corrected: number of corrected bit errors
     * - fec_uncorrectable: number of uncorrectable codewords
     * 
     * For EPON Reed-Solomon(255,239) FEC:
     * - Each codeword = 255 bytes = 2040 bits
     * - When uncorrectable, conservatively estimate 8 bit errors per codeword
     *   (RS can correct up to t=8 errors, uncorrectable means > 8 errors)
     */
    #define FEC_CODEWORD_SIZE_BITS 2040  /* 255 bytes * 8 bits */
    #define ESTIMATED_ERRORS_PER_UNCORRECTABLE_CODEWORD 8
    
    uint64_t total_bit_errors = link_stats->fec_corrected + 
                                (link_stats->fec_uncorrectable * ESTIMATED_ERRORS_PER_UNCORRECTABLE_CODEWORD);
    uint64_t total_bits = link_stats->bytes_received * 8;
    double ber = (double)total_bit_errors / (double)total_bits;
    
    return ber;
}
