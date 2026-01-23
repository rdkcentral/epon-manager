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
    
    /* Stats poller settings */
    bool stats_poller_enabled;           /**< Stats poller thread enabled (default: false) */
    uint32_t stats_poller_interval_seconds;  /**< Stats poller interval in seconds (default: 900) */
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

#endif /* EPONMGR_PERSISTENCE_H */
