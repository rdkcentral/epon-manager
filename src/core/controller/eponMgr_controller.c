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
 * @file eponMgr_controller.c
 * @brief EPON Manager Controller implementation
 */

#include "eponMgr_controller.h"
#include "epon_hal.h"
#include "eponMgr_logger.h"
#include "eponMgr_persistence.h"
#include "eponMgr_data.h"
#include "eponMgr_onu_state.h"
#include "eponMgr_queue.h"
#include "eponMgr_rbus.h"
#include "eponMgr_psm.h"
#include "eponMgr_tr181.h"
#include "eponMgr_stats_poller.h"
#include <rbus/rbus.h>

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
    
    // Core EPON Manager data context
    eponMgr_data_t *data;
    
    // Stats poller thread
    eponMgr_stats_poller_t *stats_poller;
    
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
 * 
 * Enqueues event and returns immediately to avoid blocking HAL thread.
 * 
 * @param status New ONU status
 */
static void hal_status_callback(epon_onu_status_t status) {
    if (!g_controller || !g_controller->event_queue) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ONU_STATUS;
    event.data.onu_status = status;
    
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
 * 
 * Enqueues event and returns immediately to avoid blocking HAL thread.
 * 
 * @param alarm_info Alarm information structure
 */
static void hal_alarm_callback(const epon_alarm_info_t *alarm_info) {
    if (!g_controller || !g_controller->event_queue || !alarm_info) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_ALARM;
    event.data.alarm = *alarm_info;
    
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
 * 
 * Enqueues event and returns immediately to avoid blocking HAL thread.
 * 
 * @param status Interface status information
 */
static void hal_interface_status_callback(epon_onu_interface_info_t status) {
    if (!g_controller || !g_controller->event_queue) return;
    
    // Create event
    eponMgr_event_t event;
    event.type = EPONMGR_EVENT_TYPE_INTERFACE_STATUS;
    event.data.interface_status = status;
    
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
 * 
 * @param ctrl Controller context
 * @param status ONU status value
 */
static void process_onu_status_event(eponMgr_controller_t *ctrl, epon_onu_status_t status) {
    EPONMGR_LOG_INFO("Processing ONU Status Event: status=%d\n", status);
    
    if (!ctrl || !ctrl->data) return;
    
    // Update ONU state
    eponMgr_onu_state_t *onu_state = ctrl->data->onu_state;
    if (onu_state) {
        eponMgr_onu_state_update_status(onu_state, status);
        EPONMGR_LOG_INFO("ONU status changed to: %d\n", status);
    }
    
    // TODO: Phase 7 - Report telemetry event for ONU status change
}

/**
 * @brief Check if any interface is UP
 * 
 * @param iface_list Interface list structure
 * @return true if at least one interface is UP, false otherwise
 */
static bool has_any_interface_up(epon_interface_list_t *iface_list) {
    if (!iface_list) return false;
    
    for (uint32_t i = 0; i < iface_list->interface_count; i++) {
        if (iface_list->interface[i].status == EPON_ONU_INTF_STATUS_LINK_UP) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Process interface status event
 * 
 * @param ctrl Controller context
 * @param info Interface information
 */
static void process_interface_status_event(eponMgr_controller_t *ctrl, epon_onu_interface_info_t *info) {
    EPONMGR_LOG_INFO("Processing Interface Status Event: interface=%s, status=%d\n", 
                     info->name, info->status);
    
    if (!ctrl || !ctrl->data) return;
    
    // Update interface in internal data structure
    epon_interface_list_t *iface_list = &ctrl->data->interface_list;
    
    if (iface_list) {
        // Find and update the interface
        bool found = false;
        for (uint32_t i = 0; i < iface_list->interface_count; i++) {
            if (strncmp(iface_list->interface[i].name, info->name, EPON_HAL_INTERFACE_NAME_LEN) == 0) {
                iface_list->interface[i] = *info;
                found = true;
                EPONMGR_LOG_INFO("Interface %s updated, status=%d\n", info->name, info->status);
                break;
            }
        }
        
        // Add new interface if not found and space available
        if (!found && iface_list->interface_count < EPON_HAL_MAX_INTERFACES) {
            iface_list->interface[iface_list->interface_count++] = *info;
            EPONMGR_LOG_INFO("Interface %s added, status=%d\n", info->name, info->status);
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
    
    // Step 3: Sync VEIP Interface table only if interface count changed
    if (iface_list && iface_list->interface_count != ctrl->data->if_list_count_cache) {
        EPONMGR_LOG_INFO("Interface count changed: %u -> %u, syncing VEIP table\n", 
                        ctrl->data->if_list_count_cache, iface_list->interface_count);
        if (eponMgr_tr181_sync_veip_table() != 0) {
            EPONMGR_LOG_WARN("Failed to sync VEIP Interface table\n");
        }
        // Update cache with new count
        ctrl->data->if_list_count_cache = iface_list->interface_count;
    }
    
    EPONMGR_LOG_INFO("WanManager updated: interface=%s, PHY status=%s\n", 
                info->name, phy_is_up ? "UP" : "DOWN");
    
    // TODO: Phase 7 - Report telemetry event for interface status change
}

/**
 * @brief Get standard alarm name string from enum value
 * 
 * @param alarm Alarm type enum
 * @return String representation of alarm
 */
static const char* get_standard_alarm_name(epon_hal_alarm_t alarm) {
    switch (alarm) {
        case EPON_HAL_ALARM_LOFI:
            return "LOFI";
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
        case EPON_HAL_ALARM_EQUIPMENT_FAILURE:
            return "EQUIPMENT_FAILURE";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Get vendor alarm name string from enum value
 * 
 * @param alarm Vendor alarm type enum
 * @return String representation of vendor alarm
 */
static const char* get_vendor_alarm_name(epon_vendor_alarm_t alarm) {
    switch (alarm) {
        case EPON_VENDOR_ALARM_LOS:
            return "LOS";
        case EPON_VENDOR_ALARM_DYING_GASP:
            return "DYING_GASP";
        case EPON_VENDOR_ALARM_POWER_LOW:
            return "POWER_LOW";
        case EPON_VENDOR_ALARM_POWER_HIGH:
            return "POWER_HIGH";
        case EPON_VENDOR_ALARM_TEMPERATURE:
            return "TEMPERATURE";
        case EPON_VENDOR_ALARM_FEC_THRESHOLD:
            return "FEC_THRESHOLD";
        case EPON_VENDOR_ALARM_LASER_BIAS_CURRENT:
            return "LASER_BIAS_CURRENT";
        case EPON_VENDOR_ALARM_SUPPLY_VOLTAGE:
            return "SUPPLY_VOLTAGE";
        default:
            return "UNKNOWN_VENDOR";
    }
}

/**
 * @brief Process alarm event
 * 
 * @param ctrl Controller context
 * @param alarm_info Alarm information
 */
static void process_alarm_event(eponMgr_controller_t *ctrl, epon_alarm_info_t *alarm_info) {
    if (!alarm_info) return;
    
    const char *alarm_str;
    const char *type_str;
    
    if (alarm_info->alarm_type == EPON_ALARM_TYPE_STANDARD) {
        alarm_str = get_standard_alarm_name(alarm_info->standard_alarm);
        type_str = "Standard";
    } else {
        alarm_str = get_vendor_alarm_name(alarm_info->vendor_alarm);
        type_str = "Vendor";
    }
    
    bool is_active = alarm_info->is_active;
    uint16_t llid = alarm_info->llid;
    
    if (is_active) {
        if (llid == EPON_LLID_NOT_APPLICABLE) {
            EPONMGR_LOG_WARN("%s Alarm RAISED: %s\n", type_str, alarm_str);
        } else {
            EPONMGR_LOG_WARN("%s Alarm RAISED: %s (LLID=%u)\n", type_str, alarm_str, llid);
        }
    } else {
        if (llid == EPON_LLID_NOT_APPLICABLE) {
            EPONMGR_LOG_INFO("%s Alarm CLEARED: %s\n", type_str, alarm_str);
        } else {
            EPONMGR_LOG_INFO("%s Alarm CLEARED: %s (LLID=%u)\n", type_str, alarm_str, llid);
        }
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
            process_onu_status_event(ctrl, event.data.onu_status);
            break;
            
        case EPONMGR_EVENT_TYPE_INTERFACE_STATUS:
            process_interface_status_event(ctrl, &event.data.interface_status);
            break;
            
        case EPONMGR_EVENT_TYPE_ALARM:
            process_alarm_event(ctrl, &event.data.alarm);
            break;
            
        default:
            EPONMGR_LOG_ERROR("Unknown event type: %d\n", event.type);
            break;
    }
    
    return 0;
}

/**
 * @brief Initialize the EPON Manager controller
 * 
 * Initializes all subsystems in the correct order: logger, PSM, RBUS, persistence
 * configuration, event queue, EPON data context, HAL, stats poller, and TR-181
 * parameters. Configuration values like cache_ttl are loaded from PSM.
 * 
 * @return Pointer to controller context on success, NULL on failure
 * 
 * @note Caller must call eponMgr_controller_destroy() to cleanup
 * @note Sets up HAL callbacks for status, alarm, and interface events
 * @note Performs HAL API version compatibility check
 * @note Sets global controller pointer for signal handlers
 */
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
    /* Log version information after logger initialization */
    EPONMGR_LOG_INFO("=== EPON Manager Version ====\n");
    EPONMGR_LOG_INFO("Version: %s\n", GIT_VERSION);
    EPONMGR_LOG_INFO("=============================\n");
    ctrl->logger_initialized = true;
    EPONMGR_LOG_INFO("Logger initialized\n");
    
    // Step 2: Initialize PSM interface
    if (eponMgr_psm_init() != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize PSM\n");
        goto error;
    }
    EPONMGR_LOG_INFO("PSM interface initialized\n");
    
    // Step 3: Initialize RBUS (needed for PSM operations)
    // Note: We initialize RBUS early with NULL hal_wrapper, will update later
    if (eponMgr_rbus_init("epon_manager") != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize RBUS\n");
        goto error;
    }
    ctrl->rbus_initialized = true;
    EPONMGR_LOG_INFO("RBUS initialized (handle available for PSM)\n");
    
    // Step 4: Load persistent configuration (now RBUS is available for PSM)
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
    
    // Step 5: Initialize event queue (hardcoded to 100)
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
    
    // Step 6: Initialize EPON Manager data context
    ctrl->data = (eponMgr_data_t *)malloc(sizeof(eponMgr_data_t));
    if (!ctrl->data) {
        EPONMGR_LOG_ERROR("Failed to allocate EPON data context\n");
        goto error;
    }
    
    epon_hal_config_t hal_config;
    memset(&hal_config, 0, sizeof(hal_config));
    hal_config.struct_size = sizeof(hal_config);
    hal_config.dpoe_supported = ctrl->config->dpoe_enabled;
    hal_config.status_callback = hal_status_callback;
    hal_config.alarm_callback = hal_alarm_callback;
    hal_config.interface_status_callback = hal_interface_status_callback;
    
    // Use cache TTL from configuration
    uint32_t cache_ttl = ctrl->config->cache_ttl_seconds;
    if (eponMgr_data_init(ctrl->data, &hal_config, cache_ttl) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize EPON data context\n");
        goto error;
    }
    EPONMGR_LOG_INFO("EPON data context initialized with %us cache TTL\n", cache_ttl);
    
    // Step 7: Initialize HAL
    epon_hal_return_t ret = eponMgr_data_hal_init(ctrl->data);
    if (ret != EPON_HAL_SUCCESS) {
        EPONMGR_LOG_ERROR("Failed to initialize EPON HAL: %d\n", ret);
        goto error;
    }
    EPONMGR_LOG_INFO("EPON HAL initialized successfully\n");
    
    // Step 7a: Check HAL API version compatibility
    uint32_t hal_api_version = epon_hal_get_version();
    uint32_t expected_version = EPON_HAL_API_VERSION;
    
    uint8_t hal_major = (hal_api_version >> 24) & 0xFF;
    uint8_t hal_minor = (hal_api_version >> 16) & 0xFF;
    uint8_t hal_patch = hal_api_version & 0xFFFF;
    
    uint8_t exp_major = (expected_version >> 24) & 0xFF;
    uint8_t exp_minor = (expected_version >> 16) & 0xFF;
    uint8_t exp_patch = expected_version & 0xFFFF;
    
    EPONMGR_LOG_INFO("HAL API Version: %u.%u.%u (expected %u.%u.%u)\n", 
                     hal_major, hal_minor, hal_patch, exp_major, exp_minor, exp_patch);
    
    // Check major version for compatibility (must match)
    if (hal_major != exp_major) {
        EPONMGR_LOG_ERROR("HAL API major version mismatch: %u != %u (incompatible)\n", 
                         hal_major, exp_major);
        goto error;
    }
    
    // Warn if minor version differs (backwards compatible but may lack features)
    if (hal_minor < exp_minor) {
        EPONMGR_LOG_WARN("HAL API minor version is older: %u.%u < %u.%u (may lack features)\n", 
                        hal_major, hal_minor, exp_major, exp_minor);
    } else if (hal_minor > exp_minor) {
        EPONMGR_LOG_INFO("HAL API minor version is newer: %u.%u > %u.%u (compatible)\n", 
                        hal_major, hal_minor, exp_major, exp_minor);
    }
    
    EPONMGR_LOG_INFO("HAL API version compatibility check passed\n");
    
    // Step 8: Initialize stats poller
    ctrl->stats_poller = (eponMgr_stats_poller_t *)malloc(sizeof(eponMgr_stats_poller_t));
    if (!ctrl->stats_poller) {
        EPONMGR_LOG_ERROR("Failed to allocate stats poller\n");
        goto error;
    }
    
    if (eponMgr_stats_poller_init(ctrl->stats_poller, 
                                   ctrl->data,
                                   ctrl->config->stats_poller_enabled,
                                   ctrl->config->stats_poller_interval_seconds) != 0) {
        EPONMGR_LOG_ERROR("Failed to initialize stats poller\n");
        free(ctrl->stats_poller);
        ctrl->stats_poller = NULL;
        goto error;
    }
    EPONMGR_LOG_INFO("Stats poller initialized (enabled: %s, interval: %us)\n",
                     ctrl->config->stats_poller_enabled ? "true" : "false",
                     ctrl->config->stats_poller_interval_seconds);
    
    // Step 9: Register TR-181 parameters (now data context is available)
    if (eponMgr_rbus_register_tr181() != 0) {
        EPONMGR_LOG_ERROR("Failed to register TR-181 parameters\n");
        goto error;
    }
    EPONMGR_LOG_INFO("TR-181 parameters registered\n");
    
    // Set global controller for shutdown function
    g_controller = ctrl;

    
    ctrl->running = false;
    ctrl->shutdown_requested = false;
    
    EPONMGR_LOG_INFO("EPON Manager Controller initialized successfully\n");
    return ctrl;

error:
    if (ctrl) {
        if (ctrl->stats_poller) {
            eponMgr_stats_poller_destroy(ctrl->stats_poller);
            free(ctrl->stats_poller);
        }
        if (ctrl->config) {
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

/**
 * @brief Run the main controller event loop
 * 
 * Enters the main event loop that processes events from the queue. Blocks until
 * a shutdown signal is received via eponMgr_controller_shutdown(). Starts the
 * stats poller thread if enabled in configuration.
 * 
 * @param controller Pointer to controller context
 * @return 0 on success, -1 on error
 * 
 * @note This function blocks until shutdown is requested
 * @note Processes up to 50 events per iteration to avoid starvation
 * @note Uses condition variable wait with 500ms timeout
 * @note Thread-safe: Uses internal mutex for state checks
 */
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
    
    // Start stats poller thread if enabled
    if (controller->stats_poller && controller->config->stats_poller_enabled) {
        if (eponMgr_stats_poller_start(controller->stats_poller) == 0) {
            EPONMGR_LOG_INFO("Stats poller thread started\n");
        } else {
            EPONMGR_LOG_WARN("Failed to start stats poller thread\n");
        }
    }
    
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

/**
 * @brief Request controller shutdown
 * 
 * Requests a graceful shutdown of the controller by setting the shutdown flag
 * and waking up the event loop. Can be called from signal handlers or other threads.
 * Uses the internal global controller pointer.
 * 
 * @note Thread-safe: Uses internal mutex
 * @note Can be called multiple times safely
 * @note Non-blocking: returns immediately after setting flag
 * @note Safe to call from signal handlers
 */
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

/**
 * @brief Destroy controller and cleanup all resources
 * 
 * Destroys all subsystems in reverse initialization order: RBUS, stats poller,
 * data context, event queue, configuration, and logger. Frees all allocated memory
 * and clears the global controller pointer.
 * 
 * @param controller Pointer to controller context
 * 
 * @note Safe to call with NULL pointer
 * @note Stops all threads before cleanup
 * @note Destroys synchronization primitives last
 */
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
    
    // Stop and destroy stats poller
    if (controller->stats_poller) {
        EPONMGR_LOG_INFO("Stopping stats poller...\n");
        eponMgr_stats_poller_destroy(controller->stats_poller);
        free(controller->stats_poller);
        controller->stats_poller = NULL;
    }
    
    // Deinitialize HAL before destroying data context
    if (controller->data && controller->data->hal_initialized) {
        EPONMGR_LOG_INFO("Deinitializing EPON HAL...\n");
        epon_hal_deinit();
    }
    
    // Destroy EPON data context (this also destroys all data structures)
    if (controller->data) {
        EPONMGR_LOG_INFO("Destroying EPON data context...\n");
        eponMgr_data_destroy(controller->data);
        free(controller->data);
        controller->data = NULL;
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

/**
 * @brief Check if controller is running
 * 
 * Returns the running state of the controller indicating whether the event
 * loop is active.
 * 
 * @param controller Pointer to controller context
 * @return true if running, false otherwise
 * 
 * @note Returns false if controller pointer is NULL
 * @note Does not use locking - reads volatile flag
 */
bool eponMgr_controller_is_running(const eponMgr_controller_t *controller) {
    if (!controller) return false;
    return controller->running;
}

/**
 * @brief Get stats poller from controller with lock
 * 
 * Acquires the controller mutex and returns the stats poller context.
 * Uses the global controller pointer.
 * 
 * @return Pointer to stats poller context, NULL if not initialized
 * 
 * @note Caller MUST call eponMgr_controller_unlock_stats_poller() when done
 * @note Locks the controller mutex - must unlock to avoid deadlock
 * @note Returns NULL if global controller not initialized
 */
void* eponMgr_controller_get_stats_poller(void) {
    if (!g_controller) return NULL;
    
    pthread_mutex_lock(&g_controller->mutex);
    return g_controller->stats_poller;
}

/**
 * @brief Release the lock on stats poller
 * 
 * Releases the mutex acquired by eponMgr_controller_get_stats_poller().
 * Must be called after finishing access to the stats poller.
 * 
 * @note Must be called to release lock acquired by get_stats_poller
 * @note Safe to call even if not initialized (no-op)
 */
void eponMgr_controller_unlock_stats_poller(void) {
    if (!g_controller) return;
    
    pthread_mutex_unlock(&g_controller->mutex);
}

/**
 * @brief Lock and get persistent configuration from controller
 * 
 * Acquires the controller mutex and returns the persistence configuration.
 * Uses the global controller pointer.
 * 
 * @return Pointer to persistence configuration, NULL if not initialized
 * 
 * @note Caller MUST call eponMgr_controller_unlock_persistence_config() when done
 * @note Locks the controller mutex - must unlock to avoid deadlock
 * @note Returns NULL if global controller not initialized
 */
const eponMgr_persistence_t* eponMgr_controller_lock_persistence_config(void) {
    if (!g_controller) return NULL;
    
    pthread_mutex_lock(&g_controller->mutex);
    return g_controller->config;
}

/**
 * @brief Release the lock on persistent configuration
 * 
 * Releases the mutex acquired by eponMgr_controller_lock_persistence_config().
 * Must be called after finishing access to the configuration.
 * 
 * @note Must be called to release lock acquired by lock_persistence_config
 * @note Safe to call even if not initialized (no-op)
 */
void eponMgr_controller_unlock_persistence_config(void) {
    if (!g_controller) return;
    
    pthread_mutex_unlock(&g_controller->mutex);
}
