/**
 * @file eponMgr_persistence.h
 * @brief EPON Manager persistent configuration using CCSP PSM
 * 
 * Manages persistent configuration stored in PSM (Persistent Storage Manager).
 * Configuration is stored as PSM records like "dmsb.eponmanager.DpoeEnable".
 */

#ifndef EPONMGR_PERSISTENCE_H
#define EPONMGR_PERSISTENCE_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief EPON Manager persistent configuration
 */
typedef struct {
    /* Cache settings */
    uint32_t cache_ttl_seconds;          /**< Cache TTL in seconds (default: 30) */
    
    /* HAL settings */
    bool dpoe_enabled;                   /**< DPoE support enabled */
} eponMgr_persistence_t;

/**
 * @brief Initialize persistence configuration with defaults
 * @param config Pointer to configuration structure
 * @return 0 on success, -1 on error
 */
int eponMgr_persistence_init_defaults(eponMgr_persistence_t *config);

/**
 * @brief Load persistent configuration from PSM
 * @param config Pointer to configuration structure
 * @return 0 on success, -1 on error
 */
int eponMgr_persistence_load(eponMgr_persistence_t *config);

/**
 * @brief Save configuration to PSM
 * @param config Pointer to configuration structure
 * @return 0 on success, -1 on error
 */
int eponMgr_persistence_save(const eponMgr_persistence_t *config);

/**
 * @brief Validate configuration values
 * @param config Pointer to configuration structure
 * @return 0 if valid, -1 if invalid
 */
int eponMgr_persistence_validate(const eponMgr_persistence_t *config);

#endif /* EPONMGR_PERSISTENCE_H */
