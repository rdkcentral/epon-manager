/**
 * @file eponMgr_cache.c
 * @brief EPON Manager cache implementation with thread-safe mutex protection
 * 
 * Two cache strategies:
 * 1. Statistics: TTL-based with timestamps
 * 2. Info data: Validity flag only (no expiration, invalidated on ONU status change)
 */

#include "eponMgr_cache.h"
#include <string.h>

int eponMgr_cache_init(eponMgr_cache_t *cache, uint32_t ttl_seconds) {
    if (!cache) return -1;
    
    memset(cache, 0, sizeof(eponMgr_cache_t));
    cache->ttl_seconds = ttl_seconds;
    
    if (pthread_mutex_init(&cache->mutex, NULL) != 0) {
        return -1;
    }
    
    return 0;
}

void eponMgr_cache_destroy(eponMgr_cache_t *cache) {
    if (!cache) return;
    pthread_mutex_destroy(&cache->mutex);
}

bool eponMgr_cache_is_stats_valid(time_t timestamp, uint32_t ttl_seconds) {
    if (timestamp == 0) {
        return false; /* Never cached */
    }
    
    time_t now = time(NULL);
    time_t age = now - timestamp;
    
    return (age >= 0 && age < (time_t)ttl_seconds);
}

/* Statistics cache - TTL based */

void eponMgr_cache_set_link_stats(eponMgr_cache_t *cache, const epon_hal_link_stats_t *stats) {
    if (!cache || !stats) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    memcpy(&cache->link_stats.data, stats, sizeof(epon_hal_link_stats_t));
    cache->link_stats.timestamp = time(NULL);
    cache->link_stats.valid = true;
    
    pthread_mutex_unlock(&cache->mutex);
}

bool eponMgr_cache_get_link_stats(eponMgr_cache_t *cache, epon_hal_link_stats_t *stats) {
    if (!cache || !stats) return false;
    
    pthread_mutex_lock(&cache->mutex);
    
    bool result = false;
    
    if (cache->link_stats.valid) {
        if (eponMgr_cache_is_stats_valid(cache->link_stats.timestamp, cache->ttl_seconds)) {
            /* Cache hit */
            memcpy(stats, &cache->link_stats.data, sizeof(epon_hal_link_stats_t));
            result = true;
        }
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return result;
}

void eponMgr_cache_set_transceiver_stats(eponMgr_cache_t *cache, const epon_hal_transceiver_stats_t *stats) {
    if (!cache || !stats) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    memcpy(&cache->transceiver_stats.data, stats, sizeof(epon_hal_transceiver_stats_t));
    cache->transceiver_stats.timestamp = time(NULL);
    cache->transceiver_stats.valid = true;
    
    pthread_mutex_unlock(&cache->mutex);
}

bool eponMgr_cache_get_transceiver_stats(eponMgr_cache_t *cache, epon_hal_transceiver_stats_t *stats) {
    if (!cache || !stats) return false;
    
    pthread_mutex_lock(&cache->mutex);
    
    bool result = false;
    
    if (cache->transceiver_stats.valid) {
        if (eponMgr_cache_is_stats_valid(cache->transceiver_stats.timestamp, cache->ttl_seconds)) {
            memcpy(stats, &cache->transceiver_stats.data, sizeof(epon_hal_transceiver_stats_t));
            result = true;
        }
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return result;
}

/* Info cache - validity flag based (no timestamp) */

void eponMgr_cache_set_manufacturer_info(eponMgr_cache_t *cache, const epon_onu_manufacturer_info_t *info) {
    if (!cache || !info) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    memcpy(&cache->manufacturer_info.data, info, sizeof(epon_onu_manufacturer_info_t));
    cache->manufacturer_info.valid = true;
    
    pthread_mutex_unlock(&cache->mutex);
}

bool eponMgr_cache_get_manufacturer_info(eponMgr_cache_t *cache, epon_onu_manufacturer_info_t *info) {
    if (!cache || !info) return false;
    
    pthread_mutex_lock(&cache->mutex);
    
    bool result = false;
    
    if (cache->manufacturer_info.valid) {
        /* Return cached data (no TTL check) */
        memcpy(info, &cache->manufacturer_info.data, sizeof(epon_onu_manufacturer_info_t));
        result = true;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return result;
}

void eponMgr_cache_set_link_info(eponMgr_cache_t *cache, const epon_hal_link_info_t *info) {
    if (!cache || !info) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    memcpy(&cache->link_info.data, info, sizeof(epon_hal_link_info_t));
    cache->link_info.valid = true;
    
    pthread_mutex_unlock(&cache->mutex);
}

bool eponMgr_cache_get_link_info(eponMgr_cache_t *cache, epon_hal_link_info_t *info) {
    if (!cache || !info) return false;
    
    pthread_mutex_lock(&cache->mutex);
    
    bool result = false;
    
    if (cache->link_info.valid) {
        /* Return cached data (no TTL check) */
        memcpy(info, &cache->link_info.data, sizeof(epon_hal_link_info_t));
        result = true;
    }
    
    pthread_mutex_unlock(&cache->mutex);
    return result;
}

void eponMgr_cache_invalidate_all(eponMgr_cache_t *cache) {
    if (!cache) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    /* Invalidate statistics cache */
    cache->link_stats.valid = false;
    cache->link_stats.timestamp = 0;
    
    cache->transceiver_stats.valid = false;
    cache->transceiver_stats.timestamp = 0;
    
    /* Invalidate info cache */
    cache->manufacturer_info.valid = false;
    cache->link_info.valid = false;
    
    pthread_mutex_unlock(&cache->mutex);
}

void eponMgr_cache_invalidate(eponMgr_cache_t *cache, const char *entry_name) {
    if (!cache || !entry_name) return;
    
    pthread_mutex_lock(&cache->mutex);
    
    if (strcmp(entry_name, "link_stats") == 0) {
        cache->link_stats.valid = false;
        cache->link_stats.timestamp = 0;
    }
    else if (strcmp(entry_name, "transceiver_stats") == 0) {
        cache->transceiver_stats.valid = false;
        cache->transceiver_stats.timestamp = 0;
    }
    else if (strcmp(entry_name, "manufacturer_info") == 0) {
        cache->manufacturer_info.valid = false;
    }
    else if (strcmp(entry_name, "link_info") == 0) {
        cache->link_info.valid = false;
    }
    
    pthread_mutex_unlock(&cache->mutex);
}
