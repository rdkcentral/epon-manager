/**
 * @file eponMgr_statsData.c
 * @brief EPON Manager statistics data storage with thread-safe mutex protection
 * 
 * Two data storage strategies:
 * 1. Statistics: TTL-based with timestamps
 * 2. Info data: Validity flag only (no expiration, invalidated on ONU status change)
 */

#include "eponMgr_statsData.h"
#include <string.h>

int eponMgr_statsData_init(eponMgr_statsData_t *stats_data, uint32_t ttl_seconds) {
    if (!stats_data) return -1;
    
    memset(stats_data, 0, sizeof(eponMgr_statsData_t));
    stats_data->ttl_seconds = ttl_seconds;
    
    if (pthread_mutex_init(&stats_data->mutex, NULL) != 0) {
        return -1;
    }
    
    return 0;
}

void eponMgr_statsData_destroy(eponMgr_statsData_t *stats_data) {
    if (!stats_data) return;
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

/* Statistics storage - TTL based */

void eponMgr_statsData_set_link_stats(eponMgr_statsData_t *stats_data, const epon_hal_link_stats_t *stats) {
    if (!stats_data || !stats) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    memcpy(&stats_data->link_stats.data, stats, sizeof(epon_hal_link_stats_t));
    stats_data->link_stats.timestamp = time(NULL);
    stats_data->link_stats.valid = true;
    
    pthread_mutex_unlock(&stats_data->mutex);
}

bool eponMgr_statsData_get_link_stats(eponMgr_statsData_t *stats_data, epon_hal_link_stats_t *stats) {
    if (!stats_data || !stats) return false;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    bool result = false;
    
    if (stats_data->link_stats.valid) {
        if (eponMgr_statsData_is_stats_valid(stats_data->link_stats.timestamp, stats_data->ttl_seconds)) {
            /* Data available */
            memcpy(stats, &stats_data->link_stats.data, sizeof(epon_hal_link_stats_t));
            result = true;
        }
    }
    
    pthread_mutex_unlock(&stats_data->mutex);
    return result;
}

void eponMgr_statsData_set_transceiver_stats(eponMgr_statsData_t *stats_data, const epon_hal_transceiver_stats_t *stats) {
    if (!stats_data || !stats) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    memcpy(&stats_data->transceiver_stats.data, stats, sizeof(epon_hal_transceiver_stats_t));
    stats_data->transceiver_stats.timestamp = time(NULL);
    stats_data->transceiver_stats.valid = true;
    
    pthread_mutex_unlock(&stats_data->mutex);
}

bool eponMgr_statsData_get_transceiver_stats(eponMgr_statsData_t *stats_data, epon_hal_transceiver_stats_t *stats) {
    if (!stats_data || !stats) return false;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    bool result = false;
    
    if (stats_data->transceiver_stats.valid) {
        if (eponMgr_statsData_is_stats_valid(stats_data->transceiver_stats.timestamp, stats_data->ttl_seconds)) {
            memcpy(stats, &stats_data->transceiver_stats.data, sizeof(epon_hal_transceiver_stats_t));
            result = true;
        }
    }
    
    pthread_mutex_unlock(&stats_data->mutex);
    return result;
}

/* Info storage - validity flag based (no timestamp) */

void eponMgr_statsData_set_manufacturer_info(eponMgr_statsData_t *stats_data, const epon_onu_manufacturer_info_t *info) {
    if (!stats_data || !info) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    memcpy(&stats_data->manufacturer_info.data, info, sizeof(epon_onu_manufacturer_info_t));
    stats_data->manufacturer_info.valid = true;
    
    pthread_mutex_unlock(&stats_data->mutex);
}

bool eponMgr_statsData_get_manufacturer_info(eponMgr_statsData_t *stats_data, epon_onu_manufacturer_info_t *info) {
    if (!stats_data || !info) return false;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    bool result = false;
    
    if (stats_data->manufacturer_info.valid) {
        /* Return stored data (no TTL check) */
        memcpy(info, &stats_data->manufacturer_info.data, sizeof(epon_onu_manufacturer_info_t));
        result = true;
    }
    
    pthread_mutex_unlock(&stats_data->mutex);
    return result;
}

void eponMgr_statsData_set_link_info(eponMgr_statsData_t *stats_data, const epon_hal_link_info_t *info) {
    if (!stats_data || !info) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    memcpy(&stats_data->link_info.data, info, sizeof(epon_hal_link_info_t));
    stats_data->link_info.valid = true;
    
    pthread_mutex_unlock(&stats_data->mutex);
}

bool eponMgr_statsData_get_link_info(eponMgr_statsData_t *stats_data, epon_hal_link_info_t *info) {
    if (!stats_data || !info) return false;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    bool result = false;
    
    if (stats_data->link_info.valid) {
        /* Return stored data (no TTL check) */
        memcpy(info, &stats_data->link_info.data, sizeof(epon_hal_link_info_t));
        result = true;
    }
    
    pthread_mutex_unlock(&stats_data->mutex);
    return result;
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

void eponMgr_statsData_invalidate(eponMgr_statsData_t *stats_data, const char *entry_name) {
    if (!stats_data || !entry_name) return;
    
    pthread_mutex_lock(&stats_data->mutex);
    
    if (strcmp(entry_name, "link_stats") == 0) {
        stats_data->link_stats.valid = false;
        stats_data->link_stats.timestamp = 0;
    }
    else if (strcmp(entry_name, "transceiver_stats") == 0) {
        stats_data->transceiver_stats.valid = false;
        stats_data->transceiver_stats.timestamp = 0;
    }
    else if (strcmp(entry_name, "manufacturer_info") == 0) {
        stats_data->manufacturer_info.valid = false;
    }
    else if (strcmp(entry_name, "link_info") == 0) {
        stats_data->link_info.valid = false;
    }
    
    pthread_mutex_unlock(&stats_data->mutex);
}
