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
 * @file eponMgr_telemetry.c
 * @brief EPON Manager Telemetry Module Implementation (Dummy/Stub Version)
 *
 * This is a Phase 8 dummy implementation that provides telemetry API stubs
 * without requiring actual T2 library integration. All telemetry calls are
 * logged for verification and testing.
 *
 * Production implementation would:
 * - Link against libtelemetry_msgsender.so
 * - Call real T2 APIs (t2_init, t2_event_s, t2_event_d, t2_marker)
 * - Actually send data to T2 backend
 */

#include "eponMgr_telemetry.h"
#include "eponMgr_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* Global telemetry state */
typedef struct {
    bool initialized;
    bool enabled;
    char component_name[128];
    pthread_mutex_t mutex;
    uint64_t event_count;
    uint64_t stat_count;
    uint64_t marker_count;
} eponMgr_telemetry_state_t;

static eponMgr_telemetry_state_t g_telem_state = {
    .initialized = false,
    .enabled = false,
    .component_name = {0},
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .event_count = 0,
    .stat_count = 0,
    .marker_count = 0
};

/**
 * @brief Get string representation of event type
 * 
 * @param type Event type enum
 * @return String representation of event type
 */
static const char* event_type_to_string(eponMgr_telemetry_event_type_t type) {
    switch (type) {
        case EPON_TELEM_EVENT_ONU_STATUS_CHANGE:
            return "ONU_STATUS_CHANGE";
        case EPON_TELEM_EVENT_LINK_UP:
            return "LINK_UP";
        case EPON_TELEM_EVENT_LINK_DOWN:
            return "LINK_DOWN";
        case EPON_TELEM_EVENT_ALARM_CRITICAL:
            return "ALARM_CRITICAL";
        case EPON_TELEM_EVENT_ALARM_ERROR:
            return "ALARM_ERROR";
        case EPON_TELEM_EVENT_ALARM_WARNING:
            return "ALARM_WARNING";
        case EPON_TELEM_EVENT_REGISTRATION:
            return "REGISTRATION";
        case EPON_TELEM_EVENT_DEREGISTRATION:
            return "DEREGISTRATION";
        case EPON_TELEM_EVENT_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Initialize telemetry module
 * 
 * Initializes the telemetry system. In Phase 8 dummy mode, it simply logs that
 * telemetry is initialized. In production, it would register with T2 telemetry
 * service and link against libtelemetry_msgsender.so.
 * 
 * @param component_name Name of the component (e.g., "EponManager")
 * @return 0 on success, -1 on failure
 * 
 * @note Phase 8: Dummy implementation for testing without T2 library
 * @note Production: Would call t2_init() to register with T2 service
 * @note Returns success if already initialized
 */
int eponMgr_telemetry_init(const char *component_name) {
    if (!component_name) {
        EPONMGR_LOG_ERROR("Telemetry init: NULL component name\n");
        return -1;
    }

    pthread_mutex_lock(&g_telem_state.mutex);

    if (g_telem_state.initialized) {
        EPONMGR_LOG_WARN("Telemetry already initialized\n");
        pthread_mutex_unlock(&g_telem_state.mutex);
        return 0;
    }

    strncpy(g_telem_state.component_name, component_name, 
            sizeof(g_telem_state.component_name) - 1);
    g_telem_state.component_name[sizeof(g_telem_state.component_name) - 1] = '\0';
    
    g_telem_state.enabled = true;
    g_telem_state.initialized = true;
    g_telem_state.event_count = 0;
    g_telem_state.stat_count = 0;
    g_telem_state.marker_count = 0;

    pthread_mutex_unlock(&g_telem_state.mutex);

    EPONMGR_LOG_INFO("Telemetry initialized (DUMMY MODE) for component: %s\n", 
                     component_name);
    EPONMGR_LOG_INFO("Telemetry: Using stub implementation - no actual T2 integration\n");

    return 0;
}

/**
 * @brief Cleanup telemetry module
 * 
 * Cleanup and free telemetry resources. Logs statistics summary of total
 * events, stats, and markers reported during session.
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Phase 8: Logs summary statistics
 * @note Production: Would call T2 cleanup APIs
 * @note Safe to call if not initialized (no-op)
 */
int eponMgr_telemetry_cleanup(void) {
    pthread_mutex_lock(&g_telem_state.mutex);

    if (!g_telem_state.initialized) {
        pthread_mutex_unlock(&g_telem_state.mutex);
        return 0;
    }

    EPONMGR_LOG_INFO("Telemetry cleanup - Statistics:\n");
    EPONMGR_LOG_INFO("  Total events reported: %lu\n", g_telem_state.event_count);
    EPONMGR_LOG_INFO("  Total stats reported: %lu\n", g_telem_state.stat_count);
    EPONMGR_LOG_INFO("  Total markers sent: %lu\n", g_telem_state.marker_count);

    g_telem_state.initialized = false;
    g_telem_state.enabled = false;
    memset(g_telem_state.component_name, 0, sizeof(g_telem_state.component_name));

    pthread_mutex_unlock(&g_telem_state.mutex);

    EPONMGR_LOG_INFO("Telemetry cleanup complete\n");
    return 0;
}

/**
 * @brief Report a telemetry event
 * 
 * Reports an event to the telemetry system. In Phase 8 dummy mode, this logs
 * the event with all details. In production, this would call T2 APIs like
 * t2_event_s() or t2_event_d().
 * 
 * @param event_type Type of event (ONU status, link up/down, alarm, etc.)
 * @param event_name Event name/marker (e.g., "EPON_ONU_STATUS_CHANGE")
 * @param event_data Event data string (optional, can be NULL)
 * @return 0 on success, -1 on failure
 * 
 * @note Phase 8: Logs event to console/log file
 * @note Production: Would call t2_event_s() or t2_event_d()
 * @note Thread-safe: Uses internal mutex
 * @note Increments event counter for statistics
 */
int eponMgr_telemetry_report_event(
    eponMgr_telemetry_event_type_t event_type,
    const char *event_name,
    const char *event_data)
{
    if (!event_name) {
        EPONMGR_LOG_ERROR("Telemetry report_event: NULL event name\n");
        return -1;
    }

    pthread_mutex_lock(&g_telem_state.mutex);

    if (!g_telem_state.initialized) {
        EPONMGR_LOG_ERROR("Telemetry not initialized\n");
        pthread_mutex_unlock(&g_telem_state.mutex);
        return -1;
    }

    if (!g_telem_state.enabled) {
        pthread_mutex_unlock(&g_telem_state.mutex);
        return 0;  // Silently skip if disabled
    }

    g_telem_state.event_count++;

    pthread_mutex_unlock(&g_telem_state.mutex);

    // Log the telemetry event (dummy implementation)
    if (event_data) {
        EPONMGR_LOG_INFO("TELEMETRY_EVENT[%lu]: Type=%s, Name=%s, Data=%s\n",
                        g_telem_state.event_count,
                        event_type_to_string(event_type),
                        event_name,
                        event_data);
    } else {
        EPONMGR_LOG_INFO("TELEMETRY_EVENT[%lu]: Type=%s, Name=%s\n",
                        g_telem_state.event_count,
                        event_type_to_string(event_type),
                        event_name);
    }

    /* Production code would call:
     * if (event_data) {
     *     t2_event_s(event_name, event_data);
     * } else {
     *     t2_event_d(event_name, 1);
     * }
     */

    return 0;
}

/**
 * @brief Report ONU status change event
 * 
 * Convenience function for reporting ONU status changes. Formats the event
 * data with interface name, old status, and new status.
 * 
 * @param interface_name Interface name (e.g., "veip0")
 * @param old_status Old status string
 * @param new_status New status string
 * @return 0 on success, -1 on failure
 * 
 * @note Calls eponMgr_telemetry_report_event() with formatted data
 * @note Event name: "EPON_ONU_STATUS_CHANGE"
 */
int eponMgr_telemetry_report_onu_status_change(
    const char *interface_name,
    const char *old_status,
    const char *new_status)
{
    char event_data[256];
    
    if (!interface_name || !old_status || !new_status) {
        EPONMGR_LOG_ERROR("Telemetry report_onu_status_change: NULL parameter\n");
        return -1;
    }

    snprintf(event_data, sizeof(event_data), 
             "Interface=%s, Old=%s, New=%s",
             interface_name, old_status, new_status);

    return eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_ONU_STATUS_CHANGE,
        "EPON_ONU_STATUS_CHANGE",
        event_data
    );
}

/**
 * @brief Report link up event
 * 
 * Convenience function for reporting interface link up events.
 * Formats the event data with interface name.
 * 
 * @param interface_name Interface name (e.g., "veip0")
 * @return 0 on success, -1 on failure
 * 
 * @note Calls eponMgr_telemetry_report_event() with formatted data
 * @note Event name: "EPON_LINK_UP"
 */
int eponMgr_telemetry_report_link_up(const char *interface_name) {
    char event_data[128];
    
    if (!interface_name) {
        EPONMGR_LOG_ERROR("Telemetry report_link_up: NULL interface name\n");
        return -1;
    }

    snprintf(event_data, sizeof(event_data), "Interface=%s", interface_name);

    return eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_LINK_UP,
        "EPON_LINK_UP",
        event_data
    );
}

/**
 * @brief Report link down event
 * 
 * Convenience function for reporting interface link down events.
 * Formats the event data with interface name.
 * 
 * @param interface_name Interface name (e.g., "veip0")
 * @return 0 on success, -1 on failure
 * 
 * @note Calls eponMgr_telemetry_report_event() with formatted data
 * @note Event name: "EPON_LINK_DOWN"
 */
int eponMgr_telemetry_report_link_down(const char *interface_name) {
    char event_data[128];
    
    if (!interface_name) {
        EPONMGR_LOG_ERROR("Telemetry report_link_down: NULL interface name\n");
        return -1;
    }

    snprintf(event_data, sizeof(event_data), "Interface=%s", interface_name);

    return eponMgr_telemetry_report_event(
        EPON_TELEM_EVENT_LINK_DOWN,
        "EPON_LINK_DOWN",
        event_data
    );
}

/**
 * @brief Report alarm event
 * 
 * Reports an alarm to telemetry system with severity, alarm ID, and description.
 * Automatically determines event type (critical/error/warning) based on severity.
 * 
 * @param severity Alarm severity (0=info, 1=warning, 2=error, 3+=critical)
 * @param alarm_id Alarm identifier from HAL
 * @param alarm_desc Alarm description string
 * @return 0 on success, -1 on failure
 * 
 * @note Calls eponMgr_telemetry_report_event() with formatted data
 * @note Event name: "EPON_ALARM_CRITICAL", "EPON_ALARM_ERROR", or "EPON_ALARM_WARNING"
 * @note Severity mapping: >=3=critical, 2=error, <2=warning
 */
int eponMgr_telemetry_report_alarm(
    uint32_t severity,
    uint32_t alarm_id,
    const char *alarm_desc)
{
    char event_name[128];
    char event_data[256];
    eponMgr_telemetry_event_type_t event_type;

    if (!alarm_desc) {
        alarm_desc = "Unknown alarm";
    }

    // Determine event type based on severity
    if (severity >= 3) {
        event_type = EPON_TELEM_EVENT_ALARM_CRITICAL;
        snprintf(event_name, sizeof(event_name), "EPON_ALARM_CRITICAL");
    } else if (severity == 2) {
        event_type = EPON_TELEM_EVENT_ALARM_ERROR;
        snprintf(event_name, sizeof(event_name), "EPON_ALARM_ERROR");
    } else {
        event_type = EPON_TELEM_EVENT_ALARM_WARNING;
        snprintf(event_name, sizeof(event_name), "EPON_ALARM_WARNING");
    }

    snprintf(event_data, sizeof(event_data),
             "Severity=%u, AlarmID=%u, Desc=%s",
             severity, alarm_id, alarm_desc);

    return eponMgr_telemetry_report_event(event_type, event_name, event_data);
}

/**
 * @brief Report statistics to telemetry
 */
int eponMgr_telemetry_report_stats(
    const eponMgr_telemetry_stat_t *stats,
    size_t count)
{
    if (!stats || count == 0) {
        EPONMGR_LOG_ERROR("Telemetry report_stats: Invalid parameters\n");
        return -1;
    }

    pthread_mutex_lock(&g_telem_state.mutex);

    if (!g_telem_state.initialized) {
        EPONMGR_LOG_ERROR("Telemetry not initialized\n");
        pthread_mutex_unlock(&g_telem_state.mutex);
        return -1;
    }

    if (!g_telem_state.enabled) {
        pthread_mutex_unlock(&g_telem_state.mutex);
        return 0;  // Silently skip if disabled
    }

    g_telem_state.stat_count += count;

    pthread_mutex_unlock(&g_telem_state.mutex);

    // Log statistics batch (dummy implementation)
    EPONMGR_LOG_INFO("TELEMETRY_STATS: Reporting %zu statistics\n", count);
    
    for (size_t i = 0; i < count; i++) {
        EPONMGR_LOG_DEBUG("  [%zu] %s = %lu (timestamp: %ld)\n",
                         i,
                         stats[i].stat_name,
                         stats[i].value,
                         stats[i].timestamp);
    }

    /* Production code would call:
     * for (size_t i = 0; i < count; i++) {
     *     char marker[256];
     *     snprintf(marker, sizeof(marker), "%s_split", stats[i].stat_name);
     *     t2_event_d(marker, stats[i].value);
     * }
     */

    return 0;
}

/**
 * @brief Report a single statistic
 */
int eponMgr_telemetry_report_single_stat(
    const char *stat_name,
    uint64_t value)
{
    eponMgr_telemetry_stat_t stat;
    
    if (!stat_name) {
        EPONMGR_LOG_ERROR("Telemetry report_single_stat: NULL stat name\n");
        return -1;
    }

    strncpy(stat.stat_name, stat_name, sizeof(stat.stat_name) - 1);
    stat.stat_name[sizeof(stat.stat_name) - 1] = '\0';
    stat.value = value;
    stat.timestamp = time(NULL);

    return eponMgr_telemetry_report_stats(&stat, 1);
}

/**
 * @brief Send a custom telemetry marker
 */
int eponMgr_telemetry_send_marker(const eponMgr_telemetry_marker_t *marker) {
    if (!marker) {
        EPONMGR_LOG_ERROR("Telemetry send_marker: NULL marker\n");
        return -1;
    }

    pthread_mutex_lock(&g_telem_state.mutex);

    if (!g_telem_state.initialized) {
        EPONMGR_LOG_ERROR("Telemetry not initialized\n");
        pthread_mutex_unlock(&g_telem_state.mutex);
        return -1;
    }

    if (!g_telem_state.enabled) {
        pthread_mutex_unlock(&g_telem_state.mutex);
        return 0;  // Silently skip if disabled
    }

    g_telem_state.marker_count++;

    pthread_mutex_unlock(&g_telem_state.mutex);

    // Log the marker (dummy implementation)
    EPONMGR_LOG_INFO("TELEMETRY_MARKER[%lu]: Name=%s, Value=%s, Time=%ld\n",
                    g_telem_state.marker_count,
                    marker->marker_name,
                    marker->value,
                    marker->timestamp);

    /* Production code would call:
     * t2_marker(marker->marker_name, marker->value);
     */

    return 0;
}

/**
 * @brief Check if telemetry is enabled
 */
bool eponMgr_telemetry_is_enabled(void) {
    bool enabled;
    
    pthread_mutex_lock(&g_telem_state.mutex);
    enabled = g_telem_state.enabled && g_telem_state.initialized;
    pthread_mutex_unlock(&g_telem_state.mutex);

    return enabled;
}

/**
 * @brief Enable/disable telemetry
 */
int eponMgr_telemetry_set_enabled(bool enabled) {
    pthread_mutex_lock(&g_telem_state.mutex);

    if (!g_telem_state.initialized) {
        EPONMGR_LOG_ERROR("Telemetry not initialized\n");
        pthread_mutex_unlock(&g_telem_state.mutex);
        return -1;
    }

    g_telem_state.enabled = enabled;

    pthread_mutex_unlock(&g_telem_state.mutex);

    EPONMGR_LOG_INFO("Telemetry %s\n", enabled ? "enabled" : "disabled");

    return 0;
}
