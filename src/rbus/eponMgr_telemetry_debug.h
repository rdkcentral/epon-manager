/*
 * TEMPORARY DEBUG MODULE - REMOVE BEFORE MERGING PR
 *
 * Registers a single rbus method to trigger telemetry events at runtime.
 * Useful for verifying that T2 markers are processed by the telemetry framework.
 *
 * Method path:
 *   Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()
 *
 * Input parameters:
 *   Mode    (string, required)
 *             "single"           - fire one event specified by EventId
 *             "all"              - fire every event (0..MAX-1) with dummy context
 *             "all_alarms_raise" - fire all alarm events as RAISED
 *             "all_alarms_clear" - fire all alarm events as CLEARED
 *             "all_alarms_both"  - fire all alarms RAISED then CLEARED
 *
 *   EventId (uint32, required for Mode=single)
 *             0  EPON_TELEM_ONU_LOS
 *             1  EPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED
 *             2  EPON_TELEM_ONU_REGISTRATION
 *             3  EPON_TELEM_ONU_DEREGISTRATION
 *             4  EPON_TELEM_INTF_LINK_UP
 *             5  EPON_TELEM_INTF_LINK_DOWN
 *             6  EPON_TELEM_PHY_STATUS_UP
 *             7  EPON_TELEM_PHY_STATUS_DOWN
 *             8  EPON_TELEM_ALARM_STD_LOFI
 *             9  EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD
 *             10 EPON_TELEM_ALARM_STD_ERROR_FRAME
 *             11 EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD
 *             12 EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS
 *             13 EPON_TELEM_ALARM_STD_OAM_SESSION_LOST
 *             14 EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE
 *             15 EPON_TELEM_ALARM_VENDOR_LOS
 *             16 EPON_TELEM_ALARM_VENDOR_DYING_GASP
 *             17 EPON_TELEM_ALARM_VENDOR_POWER_LOW
 *             18 EPON_TELEM_ALARM_VENDOR_POWER_HIGH
 *             19 EPON_TELEM_ALARM_VENDOR_TEMPERATURE
 *             20 EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD
 *             21 EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT
 *             22 EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE
 *             23 EPON_TELEM_SYSTEM_INIT_SUCCESS
 *             24 EPON_TELEM_SYSTEM_INIT_FAILURE
 *             25 EPON_TELEM_SYSTEM_SHUTDOWN
 *             26 EPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE
 *             27 EPON_TELEM_SYSTEM_FACTORY_RESET
 *             28 EPON_TELEM_SYSTEM_ONU_RESET
 *             29 EPON_TELEM_ERROR_HAL_CALL_FAILED
 *             30 EPON_TELEM_ERROR_EVENT_QUEUE_FULL
 *             31 EPON_TELEM_ERROR_STATS_COLLECTION_FAILED
 *             32 EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED
 *             33 EPON_TELEM_ERROR_PSM_ACCESS_FAILED
 *
 *   Ifname  (string, optional) interface name for INTF events  [default: "debug0"]
 *   Raised  (uint32, optional) 1=RAISED, 0=CLEARED for alarms  [default: 1]
 *   Llid    (uint32, optional) LLID for alarms, 65535=N/A       [default: 65535]
 *
 * Output parameters:
 *   Status      (string)  "OK" or error description
 *   EventsFired (uint32)  number of telemetry calls made
 *
 * Usage example (rbuscli):
 *   # Fire a single INTF_LINK_UP event on veip0
 *   rbuscli invokemethod Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent \
 *     Mode string single  EventId uint32 4  Ifname string veip0
 *
 *   # Fire all events with dummy context
 *   rbuscli invokemethod Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent \
 *     Mode string all
 *
 *   # Fire all alarms both RAISED and CLEARED
 *   rbuscli invokemethod Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent \
 *     Mode string all_alarms_both
 */

#ifndef EPONMGR_TELEMETRY_DEBUG_H
#define EPONMGR_TELEMETRY_DEBUG_H

#include <rbus/rbus.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register the debug telemetry DML properties with rbus.
 *
 * @param handle  An open rbus handle (same one used by eponMgr_rbus).
 * @return 0 on success, -1 on failure.
 */
int eponMgr_telemetry_debug_init(rbusHandle_t handle);

/**
 * @brief Unregister the debug telemetry DML properties.
 *
 * @param handle  The same rbus handle passed to eponMgr_telemetry_debug_init().
 */
void eponMgr_telemetry_debug_cleanup(rbusHandle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* EPONMGR_TELEMETRY_DEBUG_H */
