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
    
    return 0;
}
