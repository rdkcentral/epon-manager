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
 * @brief Self-contained telemetry event module.
 *
 * Sections (all internal symbols are file-static):
 *   1. Event descriptor table  -- marker / priority / value-format
 *   2. Value formatter         -- builds T2 value string from ctx
 *   3. HAL alarm mapper        -- HAL alarm struct -> event id
 *   4. T2 backend              -- t2_event_s() shim, log-only stub
 *   5. Dispatcher              -- public producer API entry points
 *
 * Marker names and severities track design_docs/EPON_Manager_Reference_v2.md §2.
 */

#include "eponMgr_telemetry.h"
#include "eponMgr_logger.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef HAVE_LIBT2
extern int t2_event_s(char *marker, char *value);
extern int t2_event_d(char *marker, double value);
#endif

/* ====================================================================== *
 * 1. Event descriptor table                                                *
 * ====================================================================== */

typedef enum {
    PRIO_INFO     = 0,
    PRIO_WARNING  = 1,
    PRIO_ERROR    = 2,
    PRIO_CRITICAL = 3
} priority_t;

typedef enum {
    FMT_NONE = 0,        /* value = ""                                  */
    FMT_INTF,            /* value = "Interface=<ifname>"                */
    FMT_ALARM            /* value = "RAISED"|"CLEARED"[,LLID=<n>]"      */
} fmt_t;

typedef struct {
    const char *marker;
    priority_t  priority;
    fmt_t       fmt;
} event_desc_t;

static const event_desc_t k_table[EPON_TELEM_EVENT_ID_MAX] = {
    /* §2.1 ONU Status -------------------------------------------------- */
    [EPON_TELEM_ONU_LOS]                          = { "EPON_ONU_LOS",                          PRIO_CRITICAL, FMT_NONE  },
    [EPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED]   = { "EPON_ONU_DOWNSTREAM_SIGNAL_DETECTED",   PRIO_INFO,     FMT_NONE  },
    [EPON_TELEM_ONU_REGISTRATION]                 = { "EPON_ONU_REGISTRATION",                 PRIO_INFO,     FMT_NONE  },
    [EPON_TELEM_ONU_DEREGISTRATION]               = { "EPON_ONU_DEREGISTRATION",               PRIO_WARNING,  FMT_NONE  },

    /* §2.2 Link Status ------------------------------------------------- */
    [EPON_TELEM_INTF_LINK_UP]                     = { "EPON_INTF_LINK_UP",                     PRIO_INFO,     FMT_INTF  },
    [EPON_TELEM_INTF_LINK_DOWN]                   = { "EPON_INTF_LINK_DOWN",                   PRIO_WARNING,  FMT_INTF  },
    [EPON_TELEM_PHY_STATUS_UP]                    = { "EPON_PHY_STATUS_UP",                    PRIO_INFO,     FMT_NONE  },
    [EPON_TELEM_PHY_STATUS_DOWN]                  = { "EPON_PHY_STATUS_DOWN",                  PRIO_CRITICAL, FMT_NONE  },

    /* §2.3 Standard 802.3ah alarms ------------------------------------ */
    [EPON_TELEM_ALARM_STD_LOFI]                   = { "EPON_ALARM_STD_LOFI",                   PRIO_CRITICAL, FMT_ALARM },
    [EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD]    = { "EPON_ALARM_STD_ERROR_SYMBOL_PERIOD",    PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_STD_ERROR_FRAME]            = { "EPON_ALARM_STD_ERROR_FRAME",            PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD]     = { "EPON_ALARM_STD_ERROR_FRAME_PERIOD",     PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS]    = { "EPON_ALARM_STD_ERROR_FRAME_SECONDS",    PRIO_WARNING,  FMT_ALARM },
    [EPON_TELEM_ALARM_STD_OAM_SESSION_LOST]       = { "EPON_ALARM_STD_OAM_SESSION_LOST",       PRIO_CRITICAL, FMT_ALARM },
    [EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE]      = { "EPON_ALARM_STD_EQUIPMENT_FAILURE",      PRIO_CRITICAL, FMT_ALARM },

    /* §2.4 Vendor (DPoE) alarms --------------------------------------- */
    [EPON_TELEM_ALARM_VENDOR_LOS]                 = { "EPON_ALARM_VENDOR_LOS",                 PRIO_CRITICAL, FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_DYING_GASP]          = { "EPON_ALARM_VENDOR_DYING_GASP",          PRIO_CRITICAL, FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_POWER_LOW]           = { "EPON_ALARM_VENDOR_POWER_LOW",           PRIO_WARNING,  FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_POWER_HIGH]          = { "EPON_ALARM_VENDOR_POWER_HIGH",          PRIO_WARNING,  FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_TEMPERATURE]         = { "EPON_ALARM_VENDOR_TEMPERATURE",         PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD]       = { "EPON_ALARM_VENDOR_FEC_THRESHOLD",       PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT]  = { "EPON_ALARM_VENDOR_LASER_BIAS_CURRENT",  PRIO_ERROR,    FMT_ALARM },
    [EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE]      = { "EPON_ALARM_VENDOR_SUPPLY_VOLTAGE",      PRIO_ERROR,    FMT_ALARM },

    /* §2.5 System lifecycle ------------------------------------------- */
    [EPON_TELEM_SYSTEM_INIT_SUCCESS]              = { "EPON_SYSTEM_INIT_SUCCESS",              PRIO_INFO,     FMT_NONE  },
    [EPON_TELEM_SYSTEM_INIT_FAILURE]              = { "EPON_SYSTEM_INIT_FAILURE",              PRIO_CRITICAL, FMT_NONE  },
    [EPON_TELEM_SYSTEM_SHUTDOWN]                  = { "EPON_SYSTEM_SHUTDOWN",                  PRIO_INFO,     FMT_NONE  },
    [EPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE]        = { "EPON_SYSTEM_HAL_WRONG_PON_MODE",        PRIO_CRITICAL, FMT_NONE  },
    [EPON_TELEM_SYSTEM_FACTORY_RESET]             = { "EPON_SYSTEM_FACTORY_RESET",             PRIO_WARNING,  FMT_NONE  },
    [EPON_TELEM_SYSTEM_ONU_RESET]                 = { "EPON_SYSTEM_ONU_RESET",                 PRIO_WARNING,  FMT_NONE  },

    /* §2.6 Error events ----------------------------------------------- */
    [EPON_TELEM_ERROR_HAL_CALL_FAILED]            = { "EPON_ERROR_HAL_CALL_FAILED",            PRIO_ERROR,    FMT_NONE  },
    [EPON_TELEM_ERROR_EVENT_QUEUE_FULL]           = { "EPON_ERROR_EVENT_QUEUE_FULL",           PRIO_ERROR,    FMT_NONE  },
    [EPON_TELEM_ERROR_STATS_COLLECTION_FAILED]    = { "EPON_ERROR_STATS_COLLECTION_FAILED",    PRIO_WARNING,  FMT_NONE  },
    [EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED]        = { "EPON_ERROR_RBUS_PUBLISH_FAILED",        PRIO_ERROR,    FMT_NONE  },
    [EPON_TELEM_ERROR_PSM_ACCESS_FAILED]          = { "EPON_ERROR_PSM_ACCESS_FAILED",          PRIO_ERROR,    FMT_NONE  },
};

static const event_desc_t *lookup(eponMgr_telemetry_event_id_t id)
{
    if ((unsigned)id >= EPON_TELEM_EVENT_ID_MAX) return NULL;
    if (k_table[id].marker == NULL) return NULL;
    return &k_table[id];
}

/* Rate-limit: error events (§2.6) fire at most once per second per marker.
 * Occurrences within the window are accumulated and sent as a count via
 * t2_event_d() (accumulative T2 API) when the window expires. */
#define ERROR_RATE_LIMIT_NS  1000000000LL   /* 1 second in nanoseconds */

static bool is_error_event(eponMgr_telemetry_event_id_t id)
{
    return (id >= EPON_TELEM_ERROR_HAL_CALL_FAILED &&
            id <= EPON_TELEM_ERROR_PSM_ACCESS_FAILED);
}

static int64_t timespec_diff_ns(const struct timespec *a,
                                const struct timespec *b)
{
    return (int64_t)(a->tv_sec - b->tv_sec) * (int64_t)1000000000 +
           (int64_t)(a->tv_nsec - b->tv_nsec);
}

/* ====================================================================== *
 * 2. Value formatter                                                       *
 * ====================================================================== */

typedef struct {
    const char *ifname;
    bool        raised;
    uint16_t    llid;
} ctx_t;

static int format_value(const event_desc_t *desc,
                        const ctx_t        *ctx,
                        char               *out,
                        size_t              cap)
{
    if (out == NULL || cap == 0) return -1;
    out[0] = '\0';
    int n = 0;

    switch (desc->fmt) {
    case FMT_NONE:
        return 0;
    case FMT_INTF: {
        const char *ifn = (ctx && ctx->ifname) ? ctx->ifname : "unknown";
        n = snprintf(out, cap, "Interface=%s", ifn);
        break;
    }
    case FMT_ALARM: {
        const char *state = (ctx && ctx->raised) ? "RAISED" : "CLEARED";
        if (ctx && ctx->llid != EPON_LLID_NOT_APPLICABLE) {
            n = snprintf(out, cap, "%s,LLID=%u", state, (unsigned)ctx->llid);
        } else {
            n = snprintf(out, cap, "%s", state);
        }
        break;
    }
    default:
        return -1;
    }
    if (n < 0 || (size_t)n >= cap) {
        out[cap - 1] = '\0';
        return -1;
    }
    return n;
}

/* ====================================================================== *
 * 3. HAL alarm mapper                                                      *
 * ====================================================================== */

static eponMgr_telemetry_event_id_t map_std_alarm(epon_hal_alarm_t a)
{
    switch (a) {
    case EPON_HAL_ALARM_LOFI:                 return EPON_TELEM_ALARM_STD_LOFI;
    case EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD:  return EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD;
    case EPON_HAL_ALARM_ERROR_FRAME:          return EPON_TELEM_ALARM_STD_ERROR_FRAME;
    case EPON_HAL_ALARM_ERROR_FRAME_PERIOD:   return EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD;
    case EPON_HAL_ALARM_ERROR_FRAME_SECONDS:  return EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS;
    case EPON_HAL_ALARM_OAM_SESSION_LOST:     return EPON_TELEM_ALARM_STD_OAM_SESSION_LOST;
    case EPON_HAL_ALARM_EQUIPMENT_FAILURE:    return EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE;
    default:                                  return EPON_TELEM_EVENT_ID_MAX;
    }
}

static eponMgr_telemetry_event_id_t map_vendor_alarm(epon_vendor_alarm_t a)
{
    switch (a) {
    case EPON_VENDOR_ALARM_LOS:                return EPON_TELEM_ALARM_VENDOR_LOS;
    case EPON_VENDOR_ALARM_DYING_GASP:         return EPON_TELEM_ALARM_VENDOR_DYING_GASP;
    case EPON_VENDOR_ALARM_POWER_LOW:          return EPON_TELEM_ALARM_VENDOR_POWER_LOW;
    case EPON_VENDOR_ALARM_POWER_HIGH:         return EPON_TELEM_ALARM_VENDOR_POWER_HIGH;
    case EPON_VENDOR_ALARM_TEMPERATURE:        return EPON_TELEM_ALARM_VENDOR_TEMPERATURE;
    case EPON_VENDOR_ALARM_FEC_THRESHOLD:      return EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD;
    case EPON_VENDOR_ALARM_LASER_BIAS_CURRENT: return EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT;
    case EPON_VENDOR_ALARM_SUPPLY_VOLTAGE:     return EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE;
    default:                                   return EPON_TELEM_EVENT_ID_MAX;
    }
}

static eponMgr_telemetry_event_id_t map_alarm(const epon_alarm_info_t *info)
{
    if (info == NULL) return EPON_TELEM_EVENT_ID_MAX;
    if (info->alarm_type == EPON_ALARM_TYPE_STANDARD)
        return map_std_alarm(info->standard_alarm);
    if (info->alarm_type == EPON_ALARM_TYPE_VENDOR_SPECIFIC)
        return map_vendor_alarm(info->vendor_alarm);
    return EPON_TELEM_EVENT_ID_MAX;
}

/* ====================================================================== *
 * 4. T2 backend                                                            *
 * ====================================================================== */

static const char *prio_str(priority_t p)
{
    switch (p) {
    case PRIO_INFO:     return "INFO";
    case PRIO_WARNING:  return "WARNING";
    case PRIO_ERROR:    return "ERROR";
    case PRIO_CRITICAL: return "CRITICAL";
    default:            return "UNKNOWN";
    }
}

static int t2_send(const char *marker, const char *value, priority_t prio)
{
    if (marker == NULL) return 0;
    if (value == NULL) value = "";

    EPONMGR_LOG_INFO("[T2] %-9s %s%s%s\n",
                     prio_str(prio),
                     marker,
                     value[0] ? " " : "",
                     value);

#ifdef HAVE_LIBT2
    char m[160], v[256];
    strncpy(m, marker, sizeof(m) - 1); m[sizeof(m) - 1] = '\0';
    strncpy(v, value,  sizeof(v) - 1); v[sizeof(v) - 1] = '\0';
    (void)t2_event_s(m, v);
#endif
    return 0;
}

/* Accumulative counter send — used for rate-limited error events. */
static int t2_send_count(const char *marker, uint64_t count, priority_t prio)
{
    if (marker == NULL) return 0;

    EPONMGR_LOG_INFO("[T2-D] %-9s %s count=%llu\n",
                     prio_str(prio), marker, (unsigned long long)count);

#ifdef HAVE_LIBT2
    char m[160];
    strncpy(m, marker, sizeof(m) - 1); m[sizeof(m) - 1] = '\0';
    (void)t2_event_d(m, (double)count);
#endif
    return 0;
}

/* ====================================================================== *
 * 5. Dispatcher + Public API                                               *
 * ====================================================================== */

/* Per-error-event rate-limit accumulator. */
typedef struct {
    struct timespec last_sent;   /* CLOCK_MONOTONIC time of last t2 send */
    uint64_t        pending;     /* accumulated count since last_sent     */
} err_rate_t;

static struct {
    pthread_mutex_t mtx;
    bool            initialized;
    bool            enabled;
    char            component[128];
    uint64_t        events_sent;
    uint64_t        events_dropped;
    err_rate_t      rate[EPON_TELEM_EVENT_ID_MAX]; /* zero-init: fires on first call */
} g_state = {
    .mtx         = PTHREAD_MUTEX_INITIALIZER,
    .initialized = false,
    .enabled     = false,
};

/* Error-event path: accumulate count; flush via t2_event_d when window expires. */
static int dispatch_error(eponMgr_telemetry_event_id_t id)
{
    const event_desc_t *desc = lookup(id);
    if (desc == NULL) {
        EPONMGR_LOG_WARN("telemetry: unknown error event id %d\n", (int)id);
        pthread_mutex_lock(&g_state.mtx);
        g_state.events_dropped++;
        pthread_mutex_unlock(&g_state.mtx);
        return -1;
    }

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    pthread_mutex_lock(&g_state.mtx);

    if (!g_state.initialized || !g_state.enabled) {
        g_state.events_dropped++;
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }

    err_rate_t *rs = &g_state.rate[id];
    rs->pending++;

    if (timespec_diff_ns(&now, &rs->last_sent) < ERROR_RATE_LIMIT_NS) {
        /* Within rate-limit window — accumulate without firing. */
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }

    /* Rate-limit window expired — flush accumulated count. */
    uint64_t count = rs->pending;
    rs->pending    = 0;
    rs->last_sent  = now;
    g_state.events_sent++;
    pthread_mutex_unlock(&g_state.mtx);

    return t2_send_count(desc->marker, count, desc->priority);
}

static int dispatch(eponMgr_telemetry_event_id_t id, const ctx_t *ctx)
{
    if (is_error_event(id))
        return dispatch_error(id);

    const event_desc_t *desc = lookup(id);
    if (desc == NULL) {
        EPONMGR_LOG_WARN("telemetry: unknown event id %d\n", (int)id);
        pthread_mutex_lock(&g_state.mtx);
        g_state.events_dropped++;
        pthread_mutex_unlock(&g_state.mtx);
        return -1;
    }

    pthread_mutex_lock(&g_state.mtx);
    bool ok = g_state.initialized && g_state.enabled;
    if (ok) g_state.events_sent++;
    else    g_state.events_dropped++;
    pthread_mutex_unlock(&g_state.mtx);

    if (!ok) return 0;

    char value[256];
    if (format_value(desc, ctx, value, sizeof(value)) < 0) {
        EPONMGR_LOG_WARN("telemetry: failed to format value for %s\n",
                         desc->marker);
        return -1;
    }
    return t2_send(desc->marker, value, desc->priority);
}

int eponMgr_telemetry_init(const char *component_name)
{
    if (component_name == NULL) return -1;

    pthread_mutex_lock(&g_state.mtx);
    if (g_state.initialized) {
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }
    strncpy(g_state.component, component_name, sizeof(g_state.component) - 1);
    g_state.component[sizeof(g_state.component) - 1] = '\0';
    g_state.initialized   = true;
    g_state.enabled       = true;
    g_state.events_sent   = 0;
    g_state.events_dropped = 0;
    pthread_mutex_unlock(&g_state.mtx);

    EPONMGR_LOG_INFO("telemetry: initialized component=%s (%s)\n",
                     component_name,
#ifdef HAVE_LIBT2
                     "T2 production"
#else
                     "T2 stub"
#endif
                     );
    return 0;
}

int eponMgr_telemetry_cleanup(void)
{
    pthread_mutex_lock(&g_state.mtx);
    if (!g_state.initialized) {
        pthread_mutex_unlock(&g_state.mtx);
        return 0;
    }
    EPONMGR_LOG_INFO("telemetry: cleanup (sent=%lu, dropped=%lu)\n",
                     (unsigned long)g_state.events_sent,
                     (unsigned long)g_state.events_dropped);
    g_state.initialized = false;
    g_state.enabled     = false;
    g_state.component[0] = '\0';
    pthread_mutex_unlock(&g_state.mtx);
    return 0;
}

bool eponMgr_telemetry_is_enabled(void)
{
    pthread_mutex_lock(&g_state.mtx);
    bool e = g_state.initialized && g_state.enabled;
    pthread_mutex_unlock(&g_state.mtx);
    return e;
}

int eponMgr_telemetry_set_enabled(bool enabled)
{
    pthread_mutex_lock(&g_state.mtx);
    g_state.enabled = enabled;
    pthread_mutex_unlock(&g_state.mtx);
    EPONMGR_LOG_INFO("telemetry: %s\n", enabled ? "enabled" : "disabled");
    return 0;
}

int eponMgr_telemetry_raise_simple(eponMgr_telemetry_event_id_t id)
{
    return dispatch(id, NULL);
}

int eponMgr_telemetry_raise_intf(eponMgr_telemetry_event_id_t id,
                                 const char *ifname)
{
    ctx_t ctx = { .ifname = ifname,
                  .raised = false,
                  .llid   = EPON_LLID_NOT_APPLICABLE };
    return dispatch(id, &ctx);
}

int eponMgr_telemetry_raise_alarm(const epon_alarm_info_t *info)
{
    if (info == NULL) return -1;
    eponMgr_telemetry_event_id_t id = map_alarm(info);
    if (id == EPON_TELEM_EVENT_ID_MAX) {
        EPONMGR_LOG_WARN("telemetry: unknown HAL alarm (type=%d)\n",
                         (int)info->alarm_type);
        return -1;
    }
    ctx_t ctx = { .ifname = NULL,
                  .raised = info->is_active,
                  .llid   = info->llid };
    return dispatch(id, &ctx);
}
