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
 * @file eponMgr_telemetry.h
 * @brief EPON Manager Telemetry Public API
 *
 * The rest of the EPON Manager code base only ever interacts with telemetry
 * through this header. All marker-name selection, severity mapping, value
 * formatting and T2 dispatch live inside src/telemetry/ and are private
 * to that module.
 *
 * Producer surface:
 *   - eponMgr_telemetry_raise_simple(id)
 *   - eponMgr_telemetry_raise_intf  (id, ifname)
 *   - eponMgr_telemetry_raise_alarm (const epon_alarm_info_t *info)
 *
 * See design_docs/07_Telemetry_Design.md and
 * design_docs/08_Telemetry_Design.md for details.
 */

#ifndef EPONMGR_TELEMETRY_H
#define EPONMGR_TELEMETRY_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "epon_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ *
 * Event-id catalog (1-to-1 with 08_TR181_Telemetry_Reference §2)        *
 * ------------------------------------------------------------------ */

/**
 * @brief Telemetry event identifier.
 *
 * Each enumerator maps to exactly one T2 marker name (resolved inside
 * the telemetry module). Values are stable; new ids must be appended
 * before EPON_TELEM_EVENT_ID_MAX.
 */
typedef enum {
    /* §2.1 ONU Status Events ---------------------------------------- */
    EPON_TELEM_ONU_LOS = 0,
    EPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED,
    EPON_TELEM_ONU_REGISTRATION,
    EPON_TELEM_ONU_DEREGISTRATION,

    /* §2.2 Interface Link Status Events ----------------------------- */
    EPON_TELEM_INTF_LINK_UP,
    EPON_TELEM_INTF_LINK_DOWN,
    EPON_TELEM_PHY_STATUS_UP,
    EPON_TELEM_PHY_STATUS_DOWN,

    /* §2.3 Standard IEEE 802.3ah Alarms (RAISED/CLEARED via ctx) ---- */
    EPON_TELEM_ALARM_STD_LOFI,
    EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD,
    EPON_TELEM_ALARM_STD_ERROR_FRAME,
    EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD,
    EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS,
    EPON_TELEM_ALARM_STD_OAM_SESSION_LOST,
    EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE,

    /* §2.4 Vendor-Specific (DPoE) Alarms ---------------------------- */
    EPON_TELEM_ALARM_VENDOR_LOS,
    EPON_TELEM_ALARM_VENDOR_DYING_GASP,
    EPON_TELEM_ALARM_VENDOR_POWER_LOW,
    EPON_TELEM_ALARM_VENDOR_POWER_HIGH,
    EPON_TELEM_ALARM_VENDOR_TEMPERATURE,
    EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD,
    EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT,
    EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE,

    /* §2.5 System Lifecycle ----------------------------------------- */
    EPON_TELEM_SYSTEM_INIT_SUCCESS,
    EPON_TELEM_SYSTEM_INIT_FAILURE,
    EPON_TELEM_SYSTEM_SHUTDOWN,
    EPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE,
    EPON_TELEM_SYSTEM_FACTORY_RESET,
    EPON_TELEM_SYSTEM_ONU_RESET,

    /* §2.6 Error Events --------------------------------------------- */
    EPON_TELEM_ERROR_HAL_CALL_FAILED,
    EPON_TELEM_ERROR_EVENT_QUEUE_FULL,
    EPON_TELEM_ERROR_STATS_COLLECTION_FAILED,
    EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED,
    EPON_TELEM_ERROR_PSM_ACCESS_FAILED,

    EPON_TELEM_EVENT_ID_MAX
} eponMgr_telemetry_event_id_t;


/* ------------------------------------------------------------------ *
 * Lifecycle and event producer API                                    *
 * ------------------------------------------------------------------ */

/**
 * @brief Initialize the telemetry module.
 *
 * Must be called once during application start-up before any
 * eponMgr_telemetry_raise_*() call.
 *
 * @param component_name Component name reported with every T2 event
 *                       (e.g. "EponManager"). Must not be NULL.
 * @return 0 on success, -1 on failure.
 */
int eponMgr_telemetry_init(const char *component_name);

/**
 * @brief Cleanup the telemetry module. Safe to call when not initialized.
 * @return 0 on success, -1 on failure.
 */
int eponMgr_telemetry_cleanup(void);

/**
 * @brief Raise a telemetry event that needs no caller-supplied context.
 *
 * Use this for ONU status transitions, aggregate PHY status, lifecycle
 * events, and generic error events.
 *
 * @param id Event identifier from #eponMgr_telemetry_event_id_t.
 * @return 0 on success, -1 on failure.
 */
int eponMgr_telemetry_raise_simple(eponMgr_telemetry_event_id_t id);

/**
 * @brief Raise a telemetry event that names a single interface.
 *
 * Used for EPON_TELEM_INTF_LINK_UP and EPON_TELEM_INTF_LINK_DOWN.
 *
 * @param id     Event identifier.
 * @param ifname Interface name (e.g. "veip0"). Must not be NULL.
 * @return 0 on success, -1 on failure.
 */
int eponMgr_telemetry_raise_intf(eponMgr_telemetry_event_id_t id,
                                 const char *ifname);

/**
 * @brief Raise an alarm event from a HAL alarm structure.
 *
 * The telemetry module internally maps the HAL alarm taxonomy
 * (standard / vendor + alarm enum) to the matching telemetry event id
 * and encodes the value as "RAISED" or "CLEARED" (with ",LLID=N" if
 * @p info->llid != EPON_LLID_NOT_APPLICABLE).
 *
 * Used for both standard IEEE 802.3ah alarms and vendor (DPoE) alarms.
 *
 * @param info HAL alarm information. Must not be NULL.
 * @return 0 on success, -1 on failure (incl. unknown alarm).
 */
int eponMgr_telemetry_raise_alarm(const epon_alarm_info_t *info);


#ifdef __cplusplus
}
#endif

#endif /* EPONMGR_TELEMETRY_H */
