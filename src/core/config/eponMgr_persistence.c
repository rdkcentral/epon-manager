/**
 * @file eponMgr_persistence.c
 * @brief EPON Manager persistent configuration using CCSP PSM
 */

#include "eponMgr_persistence.h"
#include "../../rbus/eponMgr_psm.h"
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
    
    printf("Loading persistent configuration from PSM...\n");
    
    /* Load cache TTL from PSM */
    uint32_t cache_ttl;
    if (eponMgr_psm_get_uint(PSM_EPON_CACHE_TTL, &cache_ttl) == 0) {
        config->cache_ttl_seconds = cache_ttl;
        printf("Loaded from PSM: cache_ttl_seconds = %u\n", cache_ttl);
    } else {
        printf("PSM: Using default cache_ttl_seconds = %u\n", config->cache_ttl_seconds);
    }
    
    /* Load DPoE enable from PSM */
    bool dpoe_enabled;
    if (eponMgr_psm_get_bool(PSM_EPON_DPOE_ENABLE, &dpoe_enabled) == 0) {
        config->dpoe_enabled = dpoe_enabled;
        printf("Loaded from PSM: dpoe_enabled = %s\n", dpoe_enabled ? "true" : "false");
    } else {
        printf("PSM: Using default dpoe_enabled = %s\n", config->dpoe_enabled ? "true" : "false");
    }
    
    /* Load stats poller enabled from PSM */
    bool stats_poller_enabled;
    if (eponMgr_psm_get_bool(PSM_EPON_STATS_POLLER_ENABLED, &stats_poller_enabled) == 0) {
        config->stats_poller_enabled = stats_poller_enabled;
        printf("Loaded from PSM: stats_poller_enabled = %s\n", stats_poller_enabled ? "true" : "false");
    } else {
        printf("PSM: Using default stats_poller_enabled = %s\n", config->stats_poller_enabled ? "true" : "false");
    }
    
    /* Load stats poller interval from PSM */
    uint32_t stats_poller_interval;
    if (eponMgr_psm_get_uint(PSM_EPON_STATS_POLLER_INTERVAL, &stats_poller_interval) == 0) {
        config->stats_poller_interval_seconds = stats_poller_interval;
        printf("Loaded from PSM: stats_poller_interval_seconds = %u\n", stats_poller_interval);
    } else {
        printf("PSM: Using default stats_poller_interval_seconds = %u\n", config->stats_poller_interval_seconds);
    }
    
    return 0;
}

int eponMgr_persistence_save(const eponMgr_persistence_t *config) {
    if (!config) {
        return -1;
    }
    
    printf("Saving persistent configuration to PSM...\n");
    
    /* Save cache TTL to PSM */
    if (eponMgr_psm_set_uint(PSM_EPON_CACHE_TTL, config->cache_ttl_seconds) != 0) {
        fprintf(stderr, "Failed to save cache_ttl_seconds to PSM\n");
        return -1;
    }
    
    /* Save DPoE enable to PSM */
    if (eponMgr_psm_set_bool(PSM_EPON_DPOE_ENABLE, config->dpoe_enabled) != 0) {
        fprintf(stderr, "Failed to save dpoe_enabled to PSM\n");
        return -1;
    }
    
    /* Save stats poller enabled to PSM */
    if (eponMgr_psm_set_bool(PSM_EPON_STATS_POLLER_ENABLED, config->stats_poller_enabled) != 0) {
        fprintf(stderr, "Failed to save stats_poller_enabled to PSM\n");
        return -1;
    }
    
    /* Save stats poller interval to PSM */
    if (eponMgr_psm_set_uint(PSM_EPON_STATS_POLLER_INTERVAL, config->stats_poller_interval_seconds) != 0) {
        fprintf(stderr, "Failed to save stats_poller_interval_seconds to PSM\n");
        return -1;
    }
    
    printf("Configuration saved to PSM successfully\n");
    return 0;
}

int eponMgr_persistence_validate(const eponMgr_persistence_t *config) {
    if (!config) {
        return -1;
    }
    
    /* Validate cache TTL */
    if (config->cache_ttl_seconds < 1 || config->cache_ttl_seconds > 300) {
        fprintf(stderr, "Invalid cache_ttl_seconds: %u (must be 1-300)\n", 
                config->cache_ttl_seconds);
        return -1;
    }
    
    /* Validate stats poller interval */
    if (config->stats_poller_interval_seconds < 60 || config->stats_poller_interval_seconds > 3600) {
        fprintf(stderr, "Invalid stats_poller_interval_seconds: %u (must be 60-3600)\n", 
                config->stats_poller_interval_seconds);
        return -1;
    }
    
    return 0;
}
