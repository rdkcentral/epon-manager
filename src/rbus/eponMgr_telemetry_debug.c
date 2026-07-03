/*
 * TEMPORARY DEBUG MODULE - REMOVE BEFORE MERGING PR
 *
 * See eponMgr_telemetry_debug.h for full documentation and usage.
 */

#include "eponMgr_telemetry_debug.h"
#include "eponMgr_telemetry.h"
#include "eponMgr_logger.h"

#include <string.h>
#include <stdint.h>
#include <rbus/rbus.h>
#include "epon_hal.h"

/* ------------------------------------------------------------------ *
 * Method path                                                         *
 * ------------------------------------------------------------------ */

#define DBG_METHOD "Device.X_RDK_Debug.EponManager.Telemetry.TriggerEvent()"

#define DEFAULT_IFNAME "debug0"
#define DEFAULT_LLID   EPON_LLID_NOT_APPLICABLE

/* ------------------------------------------------------------------ *
 * Alarm lookup tables                                                 *
 * ------------------------------------------------------------------ */

static const epon_hal_alarm_t k_std_alarm[] = {
    EPON_HAL_ALARM_LOFI,
    EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD,
    EPON_HAL_ALARM_ERROR_FRAME,
    EPON_HAL_ALARM_ERROR_FRAME_PERIOD,
    EPON_HAL_ALARM_ERROR_FRAME_SECONDS,
    EPON_HAL_ALARM_OAM_SESSION_LOST,
    EPON_HAL_ALARM_EQUIPMENT_FAILURE,
};
#define STD_ALARM_COUNT  ((int)(sizeof(k_std_alarm)  / sizeof(k_std_alarm[0])))

static const epon_vendor_alarm_t k_vendor_alarm[] = {
    EPON_VENDOR_ALARM_LOS,
    EPON_VENDOR_ALARM_DYING_GASP,
    EPON_VENDOR_ALARM_POWER_LOW,
    EPON_VENDOR_ALARM_POWER_HIGH,
    EPON_VENDOR_ALARM_TEMPERATURE,
    EPON_VENDOR_ALARM_FEC_THRESHOLD,
    EPON_VENDOR_ALARM_LASER_BIAS_CURRENT,
    EPON_VENDOR_ALARM_SUPPLY_VOLTAGE,
};
#define VENDOR_ALARM_COUNT ((int)(sizeof(k_vendor_alarm) / sizeof(k_vendor_alarm[0])))

#define STD_ALARM_FIRST   EPON_TELEM_ALARM_STD_LOFI
#define STD_ALARM_LAST    EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE
#define VEND_ALARM_FIRST  EPON_TELEM_ALARM_VENDOR_LOS
#define VEND_ALARM_LAST   EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE
#define INTF_FIRST        EPON_TELEM_INTF_LINK_UP
#define INTF_LAST         EPON_TELEM_INTF_LINK_DOWN

/* ------------------------------------------------------------------ *
 * Internal helpers                                                    *
 * ------------------------------------------------------------------ */

/* Fire a single alarm (std or vendor) with given raised state and llid */
static void fire_alarm(eponMgr_telemetry_event_id_t id, bool raised, uint16_t llid)
{
    epon_alarm_info_t info;
    memset(&info, 0, sizeof(info));
    info.is_active = raised;
    info.llid      = llid;

    if (id >= STD_ALARM_FIRST && id <= STD_ALARM_LAST) {
        info.alarm_type     = EPON_ALARM_TYPE_STANDARD;
        info.standard_alarm = k_std_alarm[id - STD_ALARM_FIRST];
    } else {
        info.alarm_type   = EPON_ALARM_TYPE_VENDOR_SPECIFIC;
        info.vendor_alarm = k_vendor_alarm[id - VEND_ALARM_FIRST];
    }
    (void)eponMgr_telemetry_raise_alarm(&info);
}

/* Fire one event from its numeric id using the supplied context */
static void fire_one(eponMgr_telemetry_event_id_t id,
                     const char *ifname, bool raised, uint16_t llid)
{
    if (id >= INTF_FIRST && id <= INTF_LAST) {
        (void)eponMgr_telemetry_raise_intf(id, ifname);
    } else if ((id >= STD_ALARM_FIRST && id <= STD_ALARM_LAST) ||
               (id >= VEND_ALARM_FIRST && id <= VEND_ALARM_LAST)) {
        fire_alarm(id, raised, llid);
    } else {
        (void)eponMgr_telemetry_raise_simple(id);
    }
}

/* Fire every alarm event with given raised state; returns count fired */
static int fire_all_alarms(bool raised, uint16_t llid)
{
    int count = 0;
    for (int i = STD_ALARM_FIRST; i <= (int)STD_ALARM_LAST; i++) {
        fire_alarm((eponMgr_telemetry_event_id_t)i, raised, llid);
        count++;
    }
    for (int i = VEND_ALARM_FIRST; i <= (int)VEND_ALARM_LAST; i++) {
        fire_alarm((eponMgr_telemetry_event_id_t)i, raised, llid);
        count++;
    }
    return count;
}

/* Helper: get string param from inParams, returns def if absent */
static const char *get_str_param(rbusObject_t inParams, const char *key, const char *def)
{
    rbusValue_t val = rbusObject_GetPropertyValue(inParams, key);
    if (val == NULL) return def;
    const char *s = rbusValue_GetString(val, NULL);
    return (s && s[0]) ? s : def;
}

/* Helper: get uint32 param from inParams, returns def if absent */
static uint32_t get_u32_param(rbusObject_t inParams, const char *key, uint32_t def)
{
    rbusValue_t val = rbusObject_GetPropertyValue(inParams, key);
    if (val == NULL) return def;
    return rbusValue_GetUInt32(val);
}

/* Helper: set a string output property on outParams */
static void set_str_out(rbusObject_t outParams, const char *key, const char *value)
{
    rbusValue_t val;
    rbusValue_Init(&val);
    rbusValue_SetString(val, value);
    rbusObject_SetValue(outParams, key, val);
    rbusValue_Release(val);
}

/* Helper: set a uint32 output property on outParams */
static void set_u32_out(rbusObject_t outParams, const char *key, uint32_t value)
{
    rbusValue_t val;
    rbusValue_Init(&val);
    rbusValue_SetUInt32(val, value);
    rbusObject_SetValue(outParams, key, val);
    rbusValue_Release(val);
}

/* ------------------------------------------------------------------ *
 * Method handler                                                      *
 * ------------------------------------------------------------------ */

static rbusError_t trigger_event_method_handler(rbusHandle_t handle,
                                                char const *methodName,
                                                rbusObject_t inParams,
                                                rbusObject_t outParams,
                                                rbusMethodAsyncHandle_t asyncHandle)
{
    (void)handle;
    (void)asyncHandle;

    EPONMGR_LOG_INFO("[telemetry_debug] method invoked: %s\n", methodName);

    /* ---- Read input params ---- */
    const char *mode   = get_str_param(inParams, "Mode",    "single");
    uint32_t    raw_id = get_u32_param (inParams, "EventId", 0);
    const char *ifname = get_str_param (inParams, "Ifname",  DEFAULT_IFNAME);
    uint32_t    raised = get_u32_param (inParams, "Raised",  1);
    uint32_t    llid   = get_u32_param (inParams, "Llid",    DEFAULT_LLID);

    bool alarm_raised = (raised != 0);
    uint16_t alarm_llid = (uint16_t)llid;

    int fired = 0;

    /* ---- Mode: single ---- */
    if (strcmp(mode, "single") == 0) {
        if (raw_id >= (uint32_t)EPON_TELEM_EVENT_ID_MAX) {
            EPONMGR_LOG_WARN("[telemetry_debug] invalid EventId=%u (max=%d)\n",
                             raw_id, EPON_TELEM_EVENT_ID_MAX - 1);
            set_str_out(outParams, "Status", "ERROR: invalid EventId");
            set_u32_out(outParams, "EventsFired", 0);
            return RBUS_ERROR_INVALID_INPUT;
        }
        EPONMGR_LOG_INFO("[telemetry_debug] single: id=%u ifname=%s raised=%u llid=%u\n",
                         raw_id, ifname, raised, llid);
        fire_one((eponMgr_telemetry_event_id_t)raw_id, ifname, alarm_raised, alarm_llid);
        fired = 1;

    /* ---- Mode: all ---- */
    } else if (strcmp(mode, "all") == 0) {
        EPONMGR_LOG_INFO("[telemetry_debug] all: firing all %d events\n",
                         EPON_TELEM_EVENT_ID_MAX);
        for (int i = 0; i < EPON_TELEM_EVENT_ID_MAX; i++) {
            fire_one((eponMgr_telemetry_event_id_t)i, ifname, alarm_raised, alarm_llid);
            fired++;
        }

    /* ---- Mode: all_alarms_raise ---- */
    } else if (strcmp(mode, "all_alarms_raise") == 0) {
        EPONMGR_LOG_INFO("[telemetry_debug] all_alarms_raise: %d std + %d vendor\n",
                         STD_ALARM_COUNT, VENDOR_ALARM_COUNT);
        fired = fire_all_alarms(true, alarm_llid);

    /* ---- Mode: all_alarms_clear ---- */
    } else if (strcmp(mode, "all_alarms_clear") == 0) {
        EPONMGR_LOG_INFO("[telemetry_debug] all_alarms_clear: %d std + %d vendor\n",
                         STD_ALARM_COUNT, VENDOR_ALARM_COUNT);
        fired = fire_all_alarms(false, alarm_llid);

    /* ---- Mode: all_alarms_both ---- */
    } else if (strcmp(mode, "all_alarms_both") == 0) {
        EPONMGR_LOG_INFO("[telemetry_debug] all_alarms_both: RAISED then CLEARED\n");
        fired += fire_all_alarms(true,  alarm_llid);
        fired += fire_all_alarms(false, alarm_llid);

    } else {
        EPONMGR_LOG_WARN("[telemetry_debug] unknown Mode='%s'\n", mode);
        set_str_out(outParams, "Status",
                    "ERROR: unknown Mode (single|all|all_alarms_raise|all_alarms_clear|all_alarms_both)");
        set_u32_out(outParams, "EventsFired", 0);
        return RBUS_ERROR_INVALID_INPUT;
    }

    EPONMGR_LOG_INFO("[telemetry_debug] done: %d events fired\n", fired);
    set_str_out(outParams, "Status", "OK");
    set_u32_out(outParams, "EventsFired", (uint32_t)fired);
    return RBUS_ERROR_SUCCESS;
}

/* ------------------------------------------------------------------ *
 * Public API                                                          *
 * ------------------------------------------------------------------ */

static rbusDataElement_t g_debug_elements[] = {
    { DBG_METHOD, RBUS_ELEMENT_TYPE_METHOD,
      { NULL, NULL, NULL, NULL, NULL, trigger_event_method_handler } },
};

#define DBG_ELEMENT_COUNT ((int)(sizeof(g_debug_elements) / sizeof(g_debug_elements[0])))

int eponMgr_telemetry_debug_init(rbusHandle_t handle)
{
    rbusError_t rc = rbus_regDataElements(handle, DBG_ELEMENT_COUNT, g_debug_elements);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_ERROR("[telemetry_debug] rbus_regDataElements failed: %d\n", rc);
        return -1;
    }
    EPONMGR_LOG_INFO("[telemetry_debug] registered method: %s\n", DBG_METHOD);
    return 0;
}

void eponMgr_telemetry_debug_cleanup(rbusHandle_t handle)
{
    rbusError_t rc = rbus_unregDataElements(handle, DBG_ELEMENT_COUNT, g_debug_elements);
    if (rc != RBUS_ERROR_SUCCESS) {
        EPONMGR_LOG_WARN("[telemetry_debug] rbus_unregDataElements failed: %d\n", rc);
    }
    EPONMGR_LOG_INFO("[telemetry_debug] debug method unregistered\n");
}
