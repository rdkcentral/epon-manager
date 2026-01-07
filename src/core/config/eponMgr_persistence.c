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
 * @file eponMgr_persistence.c
 * @brief EPON Manager persistent configuration using CCSP PSM
 */

#include "eponMgr_persistence.h"
#include "../../rbus/eponMgr_psm.h"
#include "../../logger/eponMgr_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int eponMgr_persistence_init_defaults(eponMgr_persistence_t *config) {
    if (!config) return -1;
    
    memset(config, 0, sizeof(eponMgr_persistence_t));
    
    /* Default values */
    config->cache_ttl_seconds = 30;
    config->dpoe_enabled = false;
    config->stats_poller_enabled = false;
    config->stats_poller_interval_seconds = 900;  /* 15 minutes */
    
    return 0;
}

int eponMgr_persistence_load(eponMgr_persistence_t *config) {
    if (!config) {
        return -1;
    }
    
    EPONMGR_LOG_INFO("Loading persistent configuration from PSM...\n");
    
    /* Load cache TTL from PSM */
    uint32_t cache_ttl;
    if (eponMgr_psm_get_uint(PSM_EPON_CACHE_TTL, &cache_ttl) == 0) {
        config->cache_ttl_seconds = cache_ttl;
        EPONMGR_LOG_INFO("Loaded from PSM: cache_ttl_seconds = %u\n", cache_ttl);
    } else {
        EPONMGR_LOG_WARN("PSM: Using default cache_ttl_seconds = %u\n", config->cache_ttl_seconds);
    }
    
    /* Load DPoE enable from PSM */
    bool dpoe_enabled;
    if (eponMgr_psm_get_bool(PSM_EPON_DPOE_ENABLE, &dpoe_enabled) == 0) {
        config->dpoe_enabled = dpoe_enabled;
        EPONMGR_LOG_INFO("Loaded from PSM: dpoe_enabled = %s\n", dpoe_enabled ? "true" : "false");
    } else {
        EPONMGR_LOG_WARN("PSM: Using default dpoe_enabled = %s\n", config->dpoe_enabled ? "true" : "false");
    }
    
    /* Load stats poller enabled from PSM */
    bool stats_poller_enabled;
    if (eponMgr_psm_get_bool(PSM_EPON_STATS_POLLER_ENABLED, &stats_poller_enabled) == 0) {
        config->stats_poller_enabled = stats_poller_enabled;
        EPONMGR_LOG_INFO("Loaded from PSM: stats_poller_enabled = %s\n", stats_poller_enabled ? "true" : "false");
    } else {
        EPONMGR_LOG_WARN("PSM: Using default stats_poller_enabled = %s\n", config->stats_poller_enabled ? "true" : "false");
    }
    
    /* Load stats poller interval from PSM */
    uint32_t stats_poller_interval;
    if (eponMgr_psm_get_uint(PSM_EPON_STATS_POLLER_INTERVAL, &stats_poller_interval) == 0) {
        config->stats_poller_interval_seconds = stats_poller_interval;
        EPONMGR_LOG_INFO("Loaded from PSM: stats_poller_interval_seconds = %u\n", stats_poller_interval);
    } else {
        EPONMGR_LOG_WARN("PSM: Using default stats_poller_interval_seconds = %u\n", config->stats_poller_interval_seconds);
    }
    
    return 0;
}

int eponMgr_persistence_save(const eponMgr_persistence_t *config) {
    if (!config) {
        return -1;
    }
    
    EPONMGR_LOG_INFO("Saving persistent configuration to PSM...\n");
    
    /* Save cache TTL to PSM */
    if (eponMgr_psm_set_uint(PSM_EPON_CACHE_TTL, config->cache_ttl_seconds) != 0) {
        EPONMGR_LOG_ERROR("Failed to save cache_ttl_seconds to PSM\n");
        return -1;
    }
    
    /* Save DPoE enable to PSM */
    if (eponMgr_psm_set_bool(PSM_EPON_DPOE_ENABLE, config->dpoe_enabled) != 0) {
        EPONMGR_LOG_ERROR("Failed to save dpoe_enabled to PSM\n");
        return -1;
    }
    
    /* Save stats poller enabled to PSM */
    if (eponMgr_psm_set_bool(PSM_EPON_STATS_POLLER_ENABLED, config->stats_poller_enabled) != 0) {
        EPONMGR_LOG_ERROR("Failed to save stats_poller_enabled to PSM\n");
        return -1;
    }
    
    /* Save stats poller interval to PSM */
    if (eponMgr_psm_set_uint(PSM_EPON_STATS_POLLER_INTERVAL, config->stats_poller_interval_seconds) != 0) {
        EPONMGR_LOG_ERROR("Failed to save stats_poller_interval_seconds to PSM\n");
        return -1;
    }
    
    EPONMGR_LOG_INFO("Configuration saved to PSM successfully\n");
    return 0;
}

int eponMgr_persistence_validate(const eponMgr_persistence_t *config) {
    if (!config) {
        return -1;
    }
    
    /* Validate cache TTL */
    if (config->cache_ttl_seconds < 1 || config->cache_ttl_seconds > 300) {
        EPONMGR_LOG_ERROR("Invalid cache_ttl_seconds: %u (must be 1-300)\n", 
                config->cache_ttl_seconds);
        return -1;
    }
    
    /* Validate stats poller interval */
    if (config->stats_poller_interval_seconds < 60 || config->stats_poller_interval_seconds > 3600) {
        EPONMGR_LOG_ERROR("Invalid stats_poller_interval_seconds: %u (must be 60-3600)\n", 
                config->stats_poller_interval_seconds);
        return -1;
    }
    
    return 0;
}
