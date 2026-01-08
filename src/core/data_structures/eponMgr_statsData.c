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
