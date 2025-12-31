/**
 * @file eponMgr_controller.c
 * @brief EPON Manager Controller implementation
 */

#include "eponMgr_controller.h"
#include "epon_hal.h"
#include "eponMgr_logger.h"
#include "eponMgr_persistence.h"
#include "eponMgr_hal_wrapper.h"
#include "eponMgr_onu_state.h"
#include "eponMgr_queue.h"
#include "eponMgr_rbus.h"
#include "eponMgr_psm.h"

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
    // Persistent configuration
    eponMgr_persistence_t *config;
    
    // HAL wrapper with all data structures
    eponMgr_hal_wrapper_t *hal_wrapper;
    
    // Event queue for HAL callbacks
    eponMgr_queue_t *event_queue;
    
    // Event notification
    pthread_cond_t event_cond;  /**< Condition variable for event notification */
    pthread_mutex_t event_mutex; /**< Mutex for condition variable */
    
    // Runtime state
    volatile bool running;
    volatile bool shutdown_requested;
    pthread_mutex_t mutex;
    
    // Logger state
    bool logger_initialized;
    
    // RBUS state
    bool rbus_initialized;
};

// Global controller pointer for signal handler
static eponMgr_controller_t *g_controller = NULL;



/**
 * @brief HAL status callback - called when ONU status changes
 * Enqueues event and returns immediately
 */
static void hal_status_callback(epon_onu_status_t status) {
    if (!g_controller || !g_controller->event_queue) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
    event.data.onu_status.status = status;
    
    // Enqueue event and return immediately
    if (eponMgr_queue_push(g_controller->event_queue, &event) == 0) {
        // Signal event processor to wake up
        pthread_cond_signal(&g_controller->event_cond);
    } else {
        EPONMGR_LOG_ERROR("Failed to enqueue ONU status event\n");
    }
}

/**
 * @brief HAL alarm callback - called when alarms are raised/cleared
 * Enqueues event and returns immediately
 */
static void hal_alarm_callback(epon_hal_alarm_t alarm, bool is_active) {
    if (!g_controller || !g_controller->event_queue) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ALARM;
    event.data.alarm.alarm = alarm;
    event.data.alarm.is_active = is_active;
    
    // Enqueue event and return immediately
    if (eponMgr_queue_push(g_controller->event_queue, &event) == 0) {
        // Signal event processor to wake up
        pthread_cond_signal(&g_controller->event_cond);
    } else {
        EPONMGR_LOG_ERROR("Failed to enqueue alarm event\n");
    }
}

/**
 * @brief HAL interface status callback - called when interface status changes
 * Enqueues event and returns immediately
 */
static void hal_interface_status_callback(epon_onu_interface_info_t status) {
    if (!g_controller || !g_controller->event_queue) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_INTERFACE_STATUS;
    event.data.interface_status.info = status;
    
    // Enqueue event and return immediately
    if (eponMgr_queue_push(g_controller->event_queue, &event) == 0) {
        // Signal event processor to wake up
        pthread_cond_signal(&g_controller->event_cond);
    } else {
        EPONMGR_LOG_ERROR("Failed to enqueue interface status event\n");
    }
}

/**
 * @brief Process ONU status event
 */
static void process_onu_status_event(eponMgr_controller_t *ctrl, epon_onu_status_t status) {
    EPONMGR_LOG_INFO("Processing ONU Status Event: status=%d\n", status);
    
    if (!ctrl || !ctrl->hal_wrapper) return;
    
    // Update ONU state
    eponMgr_onu_state_t *onu_state = ctrl->hal_wrapper->onu_state;
    if (onu_state) {
        eponMgr_onu_state_update_status(onu_state, status);
        
        // Invalidate cache on status change
        eponMgr_hal_wrapper_invalidate_cache(ctrl->hal_wrapper);
        EPONMGR_LOG_INFO("Cache invalidated due to ONU status change\n");
    }
    
    // TODO: Phase 7 - Report telemetry event for ONU status change
}

/**
 * @brief Check if any interface is UP
 */
static bool has_any_interface_up(eponMgr_interface_list_t *iface_list) {
    if (!iface_list) return false;
    
    for (uint32_t i = 0; i < iface_list->if_list.interface_count; i++) {
        if (iface_list->if_list.interface[i].status == EPON_ONU_INTF_STATUS_LINK_UP) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Process interface status event
 */
static void process_interface_status_event(eponMgr_controller_t *ctrl, epon_onu_interface_info_t *info) {
    EPONMGR_LOG_INFO("Processing Interface Status Event: interface=%s, status=%d\n", 
                     info->name, info->status);
    
    if (!ctrl || !ctrl->hal_wrapper) return;
    
    // Update interface list in data structures
    eponMgr_interface_list_t *iface_list = ctrl->hal_wrapper->interface_list;
    if (iface_list) {
        if (info->status == EPON_ONU_INTF_STATUS_LINK_UP) {
            eponMgr_interface_list_update(iface_list, info);
            EPONMGR_LOG_INFO("Interface %s added/updated as UP\n", info->name);
        } else {
            // Update status to down but keep in list for tracking
            eponMgr_interface_list_update(iface_list, info);
            EPONMGR_LOG_INFO("Interface %s updated as DOWN\n", info->name);
        }
    }
    
    // Phase 6/7 - Update WanManager via RBus
    // Step 1: Update virtual interface table with interface name and status
    if (eponMgr_rbus_update_virtual_interface(info->name, 
                                             info->status == EPON_ONU_INTF_STATUS_LINK_UP) != 0) {
        EPONMGR_LOG_WARN("Failed to update virtual interface %s in WanManager\n", info->name);
    }
    
    // Step 2: Check overall PHY status and notify WanManager
    // PHY UP if ANY interface is UP, PHY DOWN if ALL interfaces are DOWN
    bool phy_is_up = has_any_interface_up(iface_list);
    
    if (eponMgr_rbus_notify_wanmanager_phy_status(phy_is_up) != 0) {
        EPONMGR_LOG_WARN("Failed to notify WanManager of PHY status change\n");
    }
    
    EPONMGR_LOG_INFO("WanManager updated: interface=%s, PHY status=%s\n", 
                    info->name, phy_is_up ? "UP" : "DOWN");
    
    // TODO: Phase 7 - Report telemetry event for interface status change
}

/**
 * @brief Get alarm name string from enum value
 */
static const char* get_alarm_name(epon_hal_alarm_t alarm) {
    switch (alarm) {
        case EPON_HAL_ALARM_LOS:
            return "LOS";
        case EPON_HAL_ALARM_LOFI:
            return "LOFI";
        case EPON_HAL_ALARM_DYING_GASP:
            return "DYING_GASP";
        case EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD:
            return "ERROR_SYMBOL_PERIOD";
        case EPON_HAL_ALARM_ERROR_FRAME:
            return "ERROR_FRAME";
        case EPON_HAL_ALARM_ERROR_FRAME_PERIOD:
            return "ERROR_FRAME_PERIOD";
        case EPON_HAL_ALARM_ERROR_FRAME_SECONDS:
            return "ERROR_FRAME_SECONDS";
        case EPON_HAL_ALARM_OAM_SESSION_LOST:
            return "OAM_SESSION_LOST";
        case EPON_HAL_ALARM_POWER_LOW:
            return "POWER_LOW";
        case EPON_HAL_ALARM_POWER_HIGH:
            return "POWER_HIGH";
        case EPON_HAL_ALARM_EQUIPMENT_FAILURE:
            return "EQUIPMENT_FAILURE";
        case EPON_HAL_ALARM_TEMPERATURE:
            return "TEMPERATURE";
        case EPON_HAL_ALARM_VENDOR_SPECIFIC:
            return "VENDOR_SPECIFIC";
        case EPON_HAL_ALARM_FEC_THRESHOLD:
            return "FEC_THRESHOLD";
        case EPON_HAL_ALARM_LASER_BIAS_CURRENT:
            return "LASER_BIAS_CURRENT";
        case EPON_HAL_ALARM_SUPPLY_VOLTAGE:
            return "SUPPLY_VOLTAGE";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Process alarm event
 */
static void process_alarm_event(eponMgr_controller_t *ctrl, epon_hal_alarm_t alarm, bool is_active) {
    const char *alarm_str = get_alarm_name(alarm);
    
    if (is_active) {
        EPONMGR_LOG_WARN("Alarm RAISED: %s (%d)\n", alarm_str, alarm);
    } else {
        EPONMGR_LOG_INFO("Alarm CLEARED: %s (%d)\n", alarm_str, alarm);
    }
    
    // TODO: Phase 7 - Report telemetry event for alarm
    //   - Use T2 telemetry API to report critical/error alarms
    //   - Format: "EPONMGR_ALARM_<TYPE>_<ACTIVE|CLEARED>"
    
    (void)ctrl; // Suppress unused warning for now
}

/**
 * @brief Process one event from the queue
 * @return 0 if event processed, -1 if queue empty or error
 */
static int process_one_event(eponMgr_controller_t *ctrl) {
    if (!ctrl || !ctrl->event_queue) return -1;
    
    eponMgr_event_t event;
    int ret = eponMgr_queue_pop(ctrl->event_queue, &event);
    
    if (ret != 0) {
        // Queue is empty or error
        return -1;
    }
    
    // Process event based on type
    switch (event.type) {
        case EPONMGR_EVENT_TYPE_ONU_STATUS:
            process_onu_status_event(ctrl, event.data.onu_status.status);
            break;
            
        case EPONMGR_EVENT_TYPE_INTERFACE_STATUS:
            process_interface_status_event(ctrl, &event.data.interface_status.info);
            break;
            
        case EPONMGR_EVENT_TYPE_ALARM:
            process_alarm_event(ctrl, event.data.alarm.alarm, event.data.alarm.is_active);
            break;
            
        default:
            EPONMGR_LOG_ERROR("Unknown event type: %d\n", event.type);
            break;
    }
    
    return 0;
}

eponMgr_controller_t* eponMgr_controller_init(void) {
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
    
    // Initialize event condition variable and mutex
    if (pthread_mutex_init(&ctrl->event_mutex, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize event mutex\n");
        pthread_mutex_destroy(&ctrl->mutex);
        free(ctrl);
        return NULL;
    }
    
    if (pthread_cond_init(&ctrl->event_cond, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize event condition variable\n");
        pthread_mutex_destroy(&ctrl->event_mutex);
        pthread_mutex_destroy(&ctrl->mutex);
        free(ctrl);
        return NULL;
    }
    
    // Step 1: Initialize logger
    if (eponMgr_logger_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize logger\n");
        pthread_mutex_destroy(&ctrl->mutex);
        free(ctrl);
        return NULL;
    }
    ctrl->logger_initialized = true;
    EPONMGR_LOG_INFO("Logger initialized\n");
    
    // Step 2: Initialize PSM interface
    if (eponMgr_psm_init() != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize PSM\n");
        goto error;
    }
    EPONMGR_LOG_INFO("PSM interface initialized\n");
    
    // Step 3: Load persistent configuration
    ctrl->config = (eponMgr_persistence_t *)malloc(sizeof(eponMgr_persistence_t));
    if (!ctrl->config) {
        EPONMGR_LOG_ERROR("Failed to allocate persistence structure\n");
        goto error;
    }
    
    if (eponMgr_persistence_init_defaults(ctrl->config) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize persistence\n");
        goto error;
    }
    
    // Load configuration from PSM
    if (eponMgr_persistence_load(ctrl->config) != 0) {
        EPONMGR_LOG_WARN("Failed to load persistence from PSM, using defaults\n");
    } else {
        EPONMGR_LOG_INFO("Persistent configuration loaded from PSM\n");
    }
    
    // Override with environment variables if set
    eponMgr_persistence_load_env(ctrl->config);
    
    // Print configuration
    eponMgr_persistence_print(ctrl->config);
    
    // Step 4: Initialize event queue (hardcoded to 100)
    ctrl->event_queue = (eponMgr_queue_t *)malloc(sizeof(eponMgr_queue_t));
    if (!ctrl->event_queue) {
        EPONMGR_LOG_ERROR("Failed to allocate event queue\n");
        goto error;
    }
    if (eponMgr_queue_init(ctrl->event_queue, 100) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize event queue\n");
        goto error;
    }
    EPONMGR_LOG_INFO("Event queue initialized (capacity: 100)\n");
    
    // Step 5: Initialize HAL wrapper with data structures
    ctrl->hal_wrapper = (eponMgr_hal_wrapper_t *)malloc(sizeof(eponMgr_hal_wrapper_t));
    if (!ctrl->hal_wrapper) {
        EPONMGR_LOG_ERROR("Failed to allocate HAL wrapper\n");
        goto error;
    }
    
    epon_hal_config_t hal_config;
    memset(&hal_config, 0, sizeof(hal_config));
    hal_config.struct_size = sizeof(hal_config);
    hal_config.status_callback = hal_status_callback;
    hal_config.alarm_callback = hal_alarm_callback;
    hal_config.interface_status_callback = hal_interface_status_callback;
    
    // Use cache TTL from configuration
    uint32_t cache_ttl = ctrl->config->cache_ttl_seconds;
    if (eponMgr_hal_wrapper_init(ctrl->hal_wrapper, &hal_config, cache_ttl) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize HAL wrapper\n");
        goto error;
    }
    EPONMGR_LOG_INFO("HAL wrapper initialized with %us cache TTL\n", cache_ttl);
    
    // Step 6: Initialize HAL
    int ret = eponMgr_hal_wrapper_hal_init(ctrl->hal_wrapper);
    if (ret != EPON_HAL_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to initialize EPON HAL: %d\n", ret);
        goto error;
    }
    EPONMGR_LOG_INFO("EPON HAL initialized successfully\n");
    
    // Step 7: Initialize RBUS
    if (eponMgr_rbus_init("epon_manager", ctrl->hal_wrapper) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize RBUS\n");
        goto error;
    }
    ctrl->rbus_initialized = true;
    EPONMGR_LOG_INFO("RBUS initialized and TR-181 parameters registered\n");
    
    // Set global controller for shutdown function
    g_controller = ctrl;
    
    ctrl->running = false;
    ctrl->shutdown_requested = false;
    
    EPONMGR_LOG_INFO("EPON Manager Controller initialized successfully\n");
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
        EPONMGR_LOG_ERROR("NULL controller provided to run()\n");
        return -1;
    }
    
    pthread_mutex_lock(&controller->mutex);
    if (controller->running) {
        pthread_mutex_unlock(&controller->mutex);
        EPONMGR_LOG_WARN("Controller already running\n");
        return -1;
    }
    controller->running = true;
    controller->shutdown_requested = false;
    pthread_mutex_unlock(&controller->mutex);
    
    EPONMGR_LOG_INFO("EPON Manager Controller started\n");
    EPONMGR_LOG_INFO("Entering main event loop (send SIGTERM to stop gracefully)...\n");
    
    // Main event loop - Process events from queue
    while (!controller->shutdown_requested) {
        // Process all pending events in the queue
        int processed = 0;
        while (process_one_event(controller) == 0) {
            processed++;
            // Limit processing to avoid starvation
            if (processed >= 50) {
                EPONMGR_LOG_DEBUG("Processed %d events, yielding\n", processed);
                break;
            }
        }
        
        // Wait for event notification or timeout (500ms)
        // This is much more efficient than polling
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 0;
        timeout.tv_nsec += 500000000; // 500ms
        if (timeout.tv_nsec >= 1000000000) {
            timeout.tv_sec += 1;
            timeout.tv_nsec -= 1000000000;
        }
        
        pthread_mutex_lock(&controller->event_mutex);
        pthread_cond_timedwait(&controller->event_cond, &controller->event_mutex, &timeout);
        pthread_mutex_unlock(&controller->event_mutex);
    }
    
    EPONMGR_LOG_INFO("Controller event loop exited\n");
    
    pthread_mutex_lock(&controller->mutex);
    controller->running = false;
    pthread_mutex_unlock(&controller->mutex);
    
    return 0;
}

void eponMgr_controller_shutdown(void) {
    if (!g_controller) return;
    
    pthread_mutex_lock(&g_controller->mutex);
    if (g_controller->shutdown_requested) {
        pthread_mutex_unlock(&g_controller->mutex);
        return;
    }
    g_controller->shutdown_requested = true;
    pthread_mutex_unlock(&g_controller->mutex);
    
    // Wake up event loop if it's waiting
    pthread_cond_signal(&g_controller->event_cond);
    
    EPONMGR_LOG_INFO("Shutdown requested\n");
}

void eponMgr_controller_destroy(eponMgr_controller_t *controller) {
    if (!controller) return;
    
    EPONMGR_LOG_INFO("Destroying controller...\n");
    
    // Clear global pointer
    if (g_controller == controller) {
        g_controller = NULL;
    }
    
    // Cleanup RBUS
    if (controller->rbus_initialized) {
        EPONMGR_LOG_INFO("Cleaning up RBUS...\n");
        eponMgr_rbus_cleanup();
        controller->rbus_initialized = false;
    }
    
    // Destroy HAL wrapper (this also destroys all data structures)
    if (controller->hal_wrapper) {
        EPONMGR_LOG_INFO("Destroying HAL wrapper...\n");
        eponMgr_hal_wrapper_destroy(controller->hal_wrapper);
        free(controller->hal_wrapper);
        controller->hal_wrapper = NULL;
    }
    
    // Destroy event queue
    if (controller->event_queue) {
        EPONMGR_LOG_INFO("Destroying event queue...\n");
        eponMgr_queue_destroy(controller->event_queue);
        free(controller->event_queue);
        controller->event_queue = NULL;
    }
    
    // Destroy configuration
    if (controller->config) {
        EPONMGR_LOG_INFO("Destroying configuration...\n");
        free(controller->config);
        controller->config = NULL;
    }
    
    // Close logger
    if (controller->logger_initialized) {
        EPONMGR_LOG_INFO("Closing logger...\n");
        eponMgr_logger_close();
        controller->logger_initialized = false;
    }
    
    // Destroy synchronization primitives
    pthread_cond_destroy(&controller->event_cond);
    pthread_mutex_destroy(&controller->event_mutex);
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
