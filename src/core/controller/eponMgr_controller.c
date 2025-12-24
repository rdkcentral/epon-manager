/**
 * @file eponMgr_controller.c
 * @brief EPON Manager Controller implementation
 */

#include "eponMgr_controller.h"
#include "epon_hal.h"
#include "eponMgr_logger.h"
#include "eponMgr_config.h"
#include "eponMgr_hal_wrapper.h"
#include "eponMgr_onu_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

/**
 * @brief Controller context structure
 */
struct eponMgr_controller_context {
    // Configuration
    eponMgr_config_t *config;
    
    // HAL wrapper with all data structures
    eponMgr_hal_wrapper_t *hal_wrapper;
    
    // Runtime state
    volatile bool running;
    volatile bool shutdown_requested;
    pthread_mutex_t mutex;
    
    // Logger state
    bool logger_initialized;
};

// Global controller pointer for signal handler
static eponMgr_controller_t *g_controller = NULL;

/**
 * @brief Signal handler for graceful shutdown
 */
static void signal_handler(int signum) {
    if (g_controller) {
        EPONMGR_LOG_INFO("Received signal %d, initiating shutdown...", signum);
        eponMgr_controller_shutdown(g_controller);
    }
}

/**
 * @brief HAL status callback - called when ONU status changes
 */
static void hal_status_callback(epon_onu_status_t status) {
    if (!g_controller || !g_controller->hal_wrapper) return;
    
    EPONMGR_LOG_INFO("HAL Status Callback: status=%d", status);
    
    // Update ONU state
    eponMgr_onu_state_t *onu_state = g_controller->hal_wrapper->onu_state;
    if (onu_state) {
        eponMgr_onu_state_update_status(onu_state, status);
        
        // Invalidate cache on status change
        eponMgr_hal_wrapper_invalidate_cache(g_controller->hal_wrapper);
    }
}

/**
 * @brief HAL alarm callback - called when alarms are raised/cleared
 */
static void hal_alarm_callback(epon_hal_alarm_t alarm, bool is_active) {
    EPONMGR_LOG_INFO("HAL Alarm Callback: alarm=%d, active=%d", alarm, is_active);
    
    // TODO: Implement alarm handling in Phase 5 (Event Listener)
    // For now, just log it
}

/**
 * @brief HAL interface status callback - called when interface status changes
 */
static void hal_interface_status_callback(epon_onu_interface_info_t status) {
    EPONMGR_LOG_INFO("HAL Interface Callback: interface=%s, admin_status=%d", 
                  status.name, status.status);
    
    // TODO: Implement interface tracking in Phase 5 (Event Listener)
    // This is where we'll update WanManager
    // For now, just log it
}

eponMgr_controller_t* eponMgr_controller_init(const eponMgr_controller_config_t *config) {
    if (!config) {
        fprintf(stderr, "ERROR: NULL config provided to controller init\n");
        return NULL;
    }
    
    // Allocate controller context
    eponMgr_controller_t *ctrl = (eponMgr_controller_t *)calloc(1, sizeof(eponMgr_controller_t));
    if (!ctrl) {
        fprintf(stderr, "ERROR: Failed to allocate controller context\n");
        return NULL;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&ctrl->mutex, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize controller mutex\n");
        free(ctrl);
        return NULL;
    }
    
    // Step 1: Initialize logger
    const char *log_dir = config->enable_file_log ? "/tmp" : NULL;
    
    if (eponMgr_logger_init(log_dir, LOG_LEVEL_INFO) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize logger\n");
        pthread_mutex_destroy(&ctrl->mutex);
        free(ctrl);
        return NULL;
    }
    ctrl->logger_initialized = true;
    EPONMGR_LOG_INFO("Logger initialized");
    
    // Step 2: Load configuration
    ctrl->config = (eponMgr_config_t *)malloc(sizeof(eponMgr_config_t));
    if (!ctrl->config) {
        EPONMGR_LOG_ERROR("Failed to allocate config structure");
        goto error;
    }
    
    if (eponMgr_config_init_defaults(ctrl->config) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize config");
        goto error;
    }
    
    if (config->config_file && eponMgr_config_load_file(ctrl->config, config->config_file) != 0) {
        EPONMGR_LOG_WARN("Failed to load config file: %s, using defaults", config->config_file);
    } else {
        EPONMGR_LOG_INFO("Configuration loaded");
    }
    
    // Step 3: Initialize HAL wrapper with data structures
    ctrl->hal_wrapper = (eponMgr_hal_wrapper_t *)malloc(sizeof(eponMgr_hal_wrapper_t));
    if (!ctrl->hal_wrapper) {
        EPONMGR_LOG_ERROR("Failed to allocate HAL wrapper");
        goto error;
    }
    
    epon_hal_config_t hal_config;
    memset(&hal_config, 0, sizeof(hal_config));
    hal_config.struct_size = sizeof(hal_config);
    hal_config.status_callback = hal_status_callback;
    hal_config.alarm_callback = hal_alarm_callback;
    hal_config.interface_status_callback = hal_interface_status_callback;
    
    uint32_t cache_ttl = config->cache_ttl_seconds > 0 ? config->cache_ttl_seconds : 30;
    if (eponMgr_hal_wrapper_init(ctrl->hal_wrapper, &hal_config, cache_ttl) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize HAL wrapper");
        goto error;
    }
    EPONMGR_LOG_INFO("HAL wrapper initialized with %us cache TTL", cache_ttl);
    
    // Step 4: Initialize HAL
    int ret = eponMgr_hal_wrapper_hal_init(ctrl->hal_wrapper);
    if (ret != EPON_HAL_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to initialize EPON HAL: %d", ret);
        goto error;
    }
    EPONMGR_LOG_INFO("EPON HAL initialized successfully");
    
    // Set up signal handlers
    g_controller = ctrl;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    EPONMGR_LOG_INFO("Signal handlers registered");
    
    ctrl->running = false;
    ctrl->shutdown_requested = false;
    
    EPONMGR_LOG_INFO("EPON Manager Controller initialized successfully");
    return ctrl;

error:
    if (ctrl) {
        if (ctrl->hal_wrapper) {
            eponMgr_hal_wrapper_destroy(ctrl->hal_wrapper);
            free(ctrl->hal_wrapper);
        }
        if (ctrl->config) {
            eponMgr_config_destroy(ctrl->config);
            free(ctrl->config);
        }
        if (ctrl->logger_initialized) {
            eponMgr_logger_close();
        }
        pthread_mutex_destroy(&ctrl->mutex);
        free(ctrl);
    }
    return NULL;
}

int eponMgr_controller_run(eponMgr_controller_t *controller) {
    if (!controller) {
        EPONMGR_LOG_ERROR("NULL controller provided to run()");
        return -1;
    }
    
    pthread_mutex_lock(&controller->mutex);
    if (controller->running) {
        pthread_mutex_unlock(&controller->mutex);
        EPONMGR_LOG_WARN("Controller already running");
        return -1;
    }
    controller->running = true;
    controller->shutdown_requested = false;
    pthread_mutex_unlock(&controller->mutex);
    
    EPONMGR_LOG_INFO("EPON Manager Controller started");
    EPONMGR_LOG_INFO("Entering main event loop (Ctrl+C to stop)...");
    
    // Main event loop
    while (!controller->shutdown_requested) {
        // Sleep for 1 second
        sleep(1);
        
        // TODO: In Phase 5, this will be replaced with actual event processing
        // For now, just a simple heartbeat
        // EPONMGR_LOG_DEBUG("Controller heartbeat");
    }
    
    EPONMGR_LOG_INFO("Controller event loop exited");
    
    pthread_mutex_lock(&controller->mutex);
    controller->running = false;
    pthread_mutex_unlock(&controller->mutex);
    
    return 0;
}

void eponMgr_controller_shutdown(eponMgr_controller_t *controller) {
    if (!controller) return;
    
    pthread_mutex_lock(&controller->mutex);
    if (controller->shutdown_requested) {
        pthread_mutex_unlock(&controller->mutex);
        return;
    }
    controller->shutdown_requested = true;
    pthread_mutex_unlock(&controller->mutex);
    
    EPONMGR_LOG_INFO("Shutdown requested");
}

void eponMgr_controller_destroy(eponMgr_controller_t *controller) {
    if (!controller) return;
    
    EPONMGR_LOG_INFO("Destroying controller...");
    
    // Clear global pointer
    if (g_controller == controller) {
        g_controller = NULL;
    }
    
    // Destroy HAL wrapper (this also destroys all data structures)
    if (controller->hal_wrapper) {
        EPONMGR_LOG_INFO("Destroying HAL wrapper...");
        eponMgr_hal_wrapper_destroy(controller->hal_wrapper);
        free(controller->hal_wrapper);
        controller->hal_wrapper = NULL;
    }
    
    // Destroy configuration
    if (controller->config) {
        EPONMGR_LOG_INFO("Destroying configuration...");
        eponMgr_config_destroy(controller->config);
        free(controller->config);
        controller->config = NULL;
    }
    
    // Close logger
    if (controller->logger_initialized) {
        EPONMGR_LOG_INFO("Closing logger...");
        eponMgr_logger_close();
        controller->logger_initialized = false;
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&controller->mutex);
    
    // Free controller
    free(controller);
    
    printf("EPON Manager Controller destroyed\n");
}

bool eponMgr_controller_is_running(const eponMgr_controller_t *controller) {
    if (!controller) return false;
    return controller->running;
}

void* eponMgr_controller_get_hal_wrapper(eponMgr_controller_t *controller) {
    if (!controller) return NULL;
    return controller->hal_wrapper;
}
