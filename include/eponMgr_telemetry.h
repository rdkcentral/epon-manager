/**
 * @file eponMgr_telemetry.h
 * @brief EPON Manager Telemetry Module API
 *
 * This module provides a wrapper around RDK T2 telemetry APIs.
 * For Phase 8 implementation, this provides dummy stubs that trace
 * telemetry calls without requiring actual T2 library integration.
 *
 * In production, this would link against libtelemetry_msgsender.so
 * and call real T2 telemetry APIs.
 */

#ifndef EPONMGR_TELEMETRY_H
#define EPONMGR_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Telemetry event types
 */
typedef enum {
    EPON_TELEM_EVENT_ONU_STATUS_CHANGE,    /**< ONU status changed */
    EPON_TELEM_EVENT_LINK_UP,              /**< Interface link up */
    EPON_TELEM_EVENT_LINK_DOWN,            /**< Interface link down */
    EPON_TELEM_EVENT_ALARM_CRITICAL,       /**< Critical alarm */
    EPON_TELEM_EVENT_ALARM_ERROR,          /**< Error alarm */
    EPON_TELEM_EVENT_ALARM_WARNING,        /**< Warning alarm */
    EPON_TELEM_EVENT_REGISTRATION,         /**< ONU registration */
    EPON_TELEM_EVENT_DEREGISTRATION,       /**< ONU deregistration */
    EPON_TELEM_EVENT_ERROR                 /**< General error event */
} eponMgr_telemetry_event_type_t;

/**
 * @brief Telemetry marker data structure
 */
typedef struct {
    char marker_name[128];        /**< T2 marker name (e.g., "EPON_ONU_UP") */
    char value[256];              /**< Event value/data */
    time_t timestamp;             /**< Event timestamp */
} eponMgr_telemetry_marker_t;

/**
 * @brief Statistics telemetry data structure
 */
typedef struct {
    char stat_name[128];          /**< Statistic name */
    uint64_t value;               /**< Statistic value */
    time_t timestamp;             /**< Collection timestamp */
} eponMgr_telemetry_stat_t;

/**
 * @brief Initialize telemetry module
 *
 * This function initializes the telemetry system. In Phase 8 dummy mode,
 * it simply logs that telemetry is initialized. In production, it would
 * register with T2 telemetry service.
 *
 * @param component_name Name of the component (e.g., "EponManager")
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_init(const char *component_name);

/**
 * @brief Cleanup telemetry module
 *
 * Cleanup and free telemetry resources.
 *
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_cleanup(void);

/**
 * @brief Report a telemetry event
 *
 * Reports an event to the telemetry system. In Phase 8 dummy mode,
 * this logs the event with all details. In production, this would
 * call T2 APIs like t2_event_s() or t2_event_d().
 *
 * @param event_type Type of event
 * @param event_name Event name/marker
 * @param event_data Event data (optional, can be NULL)
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_event(
    eponMgr_telemetry_event_type_t event_type,
    const char *event_name,
    const char *event_data
);

/**
 * @brief Report ONU status change event
 *
 * Convenience function for reporting ONU status changes.
 *
 * @param interface_name Interface name (e.g., "veip0")
 * @param old_status Old status string
 * @param new_status New status string
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_onu_status_change(
    const char *interface_name,
    const char *old_status,
    const char *new_status
);

/**
 * @brief Report link up event
 *
 * Convenience function for reporting interface link up.
 *
 * @param interface_name Interface name (e.g., "veip0")
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_link_up(const char *interface_name);

/**
 * @brief Report link down event
 *
 * Convenience function for reporting interface link down.
 *
 * @param interface_name Interface name (e.g., "veip0")
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_link_down(const char *interface_name);

/**
 * @brief Report alarm event
 *
 * Reports an alarm to telemetry system.
 *
 * @param severity Alarm severity (0=info, 1=warning, 2=error, 3=critical)
 * @param alarm_id Alarm identifier
 * @param alarm_desc Alarm description
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_alarm(
    uint32_t severity,
    uint32_t alarm_id,
    const char *alarm_desc
);

/**
 * @brief Report statistics to telemetry
 *
 * Reports periodic statistics to telemetry system. In Phase 8 dummy mode,
 * this logs the statistics. In production, this would batch-report to T2.
 *
 * @param stats Array of statistics
 * @param count Number of statistics in array
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_stats(
    const eponMgr_telemetry_stat_t *stats,
    size_t count
);

/**
 * @brief Report a single statistic
 *
 * Reports a single statistic value.
 *
 * @param stat_name Statistic name (e.g., "EPON_RxBytes")
 * @param value Statistic value
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_report_single_stat(
    const char *stat_name,
    uint64_t value
);

/**
 * @brief Send a custom telemetry marker
 *
 * Sends a custom telemetry marker to T2. In Phase 8 dummy mode,
 * this logs the marker. In production, this calls t2_marker().
 *
 * @param marker Marker data structure
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_send_marker(const eponMgr_telemetry_marker_t *marker);

/**
 * @brief Check if telemetry is enabled
 *
 * @return true if telemetry is enabled, false otherwise
 */
bool eponMgr_telemetry_is_enabled(void);

/**
 * @brief Enable/disable telemetry
 *
 * @param enabled true to enable, false to disable
 * @return 0 on success, -1 on failure
 */
int eponMgr_telemetry_set_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* EPONMGR_TELEMETRY_H */
