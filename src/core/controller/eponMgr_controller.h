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

/**
 * @brief Controller configuration
 */
typedef struct {
    const char *config_file;        /**< Path to configuration file */
    bool enable_console_log;        /**< Enable console logging */
    bool enable_file_log;           /**< Enable file logging */
    uint32_t cache_ttl_seconds;     /**< Cache TTL in seconds (default 30) */
} eponMgr_controller_config_t;

/**
 * @brief Controller context
 */
typedef struct eponMgr_controller_context eponMgr_controller_t;

/**
 * @brief Initialize the EPON Manager controller
 * 
 * This initializes all subsystems in the correct order:
 * 1. Logger
 * 2. Configuration
 * 3. HAL wrapper with data structures
 * 4. HAL initialization
 * 
 * @param config Controller configuration
 * @return Pointer to controller context on success, NULL on failure
 */
eponMgr_controller_t* eponMgr_controller_init(const eponMgr_controller_config_t *config);

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
 * to request a graceful shutdown.
 * 
 * @param controller Pointer to controller context
 */
void eponMgr_controller_shutdown(eponMgr_controller_t *controller);

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
 * @brief Get HAL wrapper from controller
 * 
 * @param controller Pointer to controller context
 * @return Pointer to HAL wrapper context, NULL on error
 */
void* eponMgr_controller_get_hal_wrapper(eponMgr_controller_t *controller);

#endif /* EPONMGR_CONTROLLER_H */
