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
 * @file eponMgr_controller.h
 * @brief EPON Manager Controller - Main component
 * 
 * Responsibilities:
 * - Initialize all subsystems
 * - Coordinate between components
 * - Main event loop and lifecycle management
 * - HAL callback registration
 * - Signal handling for graceful shutdown
 */

#ifndef EPONMGR_CONTROLLER_H
#define EPONMGR_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>
#include "eponMgr_persistence.h"

/**
 * @brief Controller context
 */
typedef struct eponMgr_controller_context eponMgr_controller_t;

/**
 * @brief Initialize the EPON Manager controller
 * 
 * This initializes all subsystems in the correct order:
 * 1. Logger
 * 2. Configuration (loads from default path /etc/epon_manager.conf)
 * 3. HAL wrapper with data structures
 * 4. HAL initialization
 * 
 * Configuration values like cache_ttl are loaded from the config file.
 * 
 * @return Pointer to controller context on success, NULL on failure
 */
eponMgr_controller_t* eponMgr_controller_init(void);

/**
 * @brief Run the main controller event loop
 * 
 * This function blocks until a shutdown signal is received.
 * 
 * @param controller Pointer to controller context
 * @return 0 on success, -1 on error
 */
int eponMgr_controller_run(eponMgr_controller_t *controller);

/**
 * @brief Request controller shutdown
 * 
 * This function can be called from signal handlers or other threads
 * to request a graceful shutdown. Uses the internal global controller.
 */
void eponMgr_controller_shutdown(void);

/**
 * @brief Destroy controller and cleanup all resources
 * 
 * This destroys all subsystems in reverse order and frees all resources.
 * 
 * @param controller Pointer to controller context
 */
void eponMgr_controller_destroy(eponMgr_controller_t *controller);

/**
 * @brief Check if controller is running
 * 
 * @param controller Pointer to controller context
 * @return true if running, false otherwise
 */
bool eponMgr_controller_is_running(const eponMgr_controller_t *controller);

/**
 * @brief Get stats poller from controller with lock
 * 
 * Acquires the controller mutex and returns the stats poller.
 * Caller MUST call eponMgr_controller_unlock_stats_poller() when done.
 * 
 * @return Pointer to stats poller context, NULL if not initialized
 * @note Caller must call unlock to release the mutex
 */
void* eponMgr_controller_get_stats_poller(void);

/**
 * @brief Release the lock on stats poller
 * 
 * Releases the mutex acquired by eponMgr_controller_get_stats_poller().
 */
void eponMgr_controller_unlock_stats_poller(void);

/**
 * @brief Lock and get persistent configuration from controller
 * 
 * Acquires the controller mutex and returns the persistence configuration.
 * Caller MUST call eponMgr_controller_unlock_persistence_config() when done.
 * 
 * @return Pointer to persistence configuration, NULL if not initialized
 * @note Caller must call unlock to release the mutex
 */
const eponMgr_persistence_t* eponMgr_controller_lock_persistence_config(void);

/**
 * @brief Release the lock on persistent configuration
 * 
 * Releases the mutex acquired by eponMgr_controller_lock_persistence_config().
 */
void eponMgr_controller_unlock_persistence_config(void);

#endif /* EPONMGR_CONTROLLER_H */
