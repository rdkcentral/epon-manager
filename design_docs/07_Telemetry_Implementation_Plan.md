# EPON Manager — Telemetry & Harvester Implementation Plan

**Version:** 1.0  |  **Date:** April 30, 2026
**Owner:** EPON Manager team
**Companion document:** [08_Telemetry_Design.md](08_Telemetry_Design.md)
**Source-of-truth spec:** [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) §2 (Telemetry) and §3 (Harvester)

---

## 1. Goal

Replace the current scaffold (`src/telemetry/eponMgr_telemetry.{c,h}`) with a
production-grade telemetry layer that:

1. **Encapsulates all marker selection, formatting, severity mapping, T2
   dispatch, and Avro/Harvester encoding inside `src/telemetry/`.**
2. **Exposes the rest of the code base only three event-firing APIs** plus
   three harvester APIs. Callers pass an **event id**; they never construct
   marker names or value strings.
3. **Implements the full set of 34 telemetry markers** defined in
   [EPON_Manager_Reference_v2.md §2](EPON_Manager_Reference_v2.md#2-telemetry-events)
   (ONU status, interface link, IEEE 802.3ah alarms, vendor/DPoE alarms,
   system lifecycle, error events).
4. **Implements the `EPONTelemetryDiagnostics` Avro report** described in
   [EPON_Manager_Reference_v2.md §3](EPON_Manager_Reference_v2.md#3-harvester-periodic-report-avro),
   periodic at `StatsPoller.PollingInterval` (default 900 s), published via
   the same CCSP/WebPA pattern used by `rdk-xdslmanager`.

---

## 2. Decisions (frozen)

| # | Topic | Decision |
|---|-------|----------|
| 1 | Avro encoder | Link against **`libavro-c`** (already in SDK). |
| 2 | Harvester sink | Replicate `rdk-xdslmanager` flow: build serialized buffer (MAGIC + UUID + schema MD5 + Avro bin), send via `sendWebpaMsg()` over CCSP/WebPA. Implementation reference: `rdkxdslmanager/source/TR-181/integration_src.shared/xdsl_report.c`. |
| 3 | Producer API | **Three** entry points only — `_raise_simple()`, `_raise_intf()`, `_raise_alarm()` — backed by a single `_raise(id, ctx)` dispatcher. |
| 4 | Alarm raised/cleared | **One** event id per alarm; the `RAISED`/`CLEARED` state is encoded in the marker value (per design doc §2.3 note). |
| 5 | Existing `eponMgr_telemetry.{c,h}` | Treated as a template — free to rewrite. Old per-stat marker APIs are removed (replaced by harvester). |

---

## 3. Public API (final shape)

Declared in [`include/eponMgr_telemetry.h`](../include/eponMgr_telemetry.h).
Everything below is the entire surface visible to non-telemetry code.

```c
/* ---- Event id catalog (1-to-1 with Reference §2) ----------------- */
typedef enum {
    /* §2.1 ONU Status Events */
    EPON_TELEM_ONU_LOS,
    EPON_TELEM_ONU_DOWNSTREAM_SIGNAL_DETECTED,
    EPON_TELEM_ONU_REGISTRATION,
    EPON_TELEM_ONU_DEREGISTRATION,

    /* §2.2 Interface Link Status Events */
    EPON_TELEM_INTF_LINK_UP,
    EPON_TELEM_INTF_LINK_DOWN,
    EPON_TELEM_PHY_STATUS_UP,
    EPON_TELEM_PHY_STATUS_DOWN,

    /* §2.3 Standard IEEE 802.3ah Alarms (RAISED/CLEARED via ctx) */
    EPON_TELEM_ALARM_STD_LOFI,
    EPON_TELEM_ALARM_STD_ERROR_SYMBOL_PERIOD,
    EPON_TELEM_ALARM_STD_ERROR_FRAME,
    EPON_TELEM_ALARM_STD_ERROR_FRAME_PERIOD,
    EPON_TELEM_ALARM_STD_ERROR_FRAME_SECONDS,
    EPON_TELEM_ALARM_STD_OAM_SESSION_LOST,
    EPON_TELEM_ALARM_STD_EQUIPMENT_FAILURE,

    /* §2.4 Vendor-Specific (DPoE) Alarms */
    EPON_TELEM_ALARM_VENDOR_LOS,
    EPON_TELEM_ALARM_VENDOR_DYING_GASP,
    EPON_TELEM_ALARM_VENDOR_POWER_LOW,
    EPON_TELEM_ALARM_VENDOR_POWER_HIGH,
    EPON_TELEM_ALARM_VENDOR_TEMPERATURE,
    EPON_TELEM_ALARM_VENDOR_FEC_THRESHOLD,
    EPON_TELEM_ALARM_VENDOR_LASER_BIAS_CURRENT,
    EPON_TELEM_ALARM_VENDOR_SUPPLY_VOLTAGE,

    /* §2.5 System Lifecycle */
    EPON_TELEM_SYSTEM_INIT_SUCCESS,
    EPON_TELEM_SYSTEM_INIT_FAILURE,
    EPON_TELEM_SYSTEM_SHUTDOWN,
    EPON_TELEM_SYSTEM_HAL_WRONG_PON_MODE,
    EPON_TELEM_SYSTEM_FACTORY_RESET,
    EPON_TELEM_SYSTEM_ONU_RESET,

    /* §2.6 Error Events */
    EPON_TELEM_ERROR_HAL_CALL_FAILED,
    EPON_TELEM_ERROR_EVENT_QUEUE_FULL,
    EPON_TELEM_ERROR_STATS_COLLECTION_FAILED,
    EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED,
    EPON_TELEM_ERROR_PSM_ACCESS_FAILED,

    EPON_TELEM_EVENT_ID_MAX
} eponMgr_telemetry_event_id_t;


/* ---- Caller-supplied context (only fields relevant to id are read) -- */
typedef struct {
    const char *interface_name;   /* link / intf events       */
    const char *old_status;       /* ONU status change        */
    const char *new_status;       /* ONU status change        */
    bool        alarm_raised;     /* RAISED (true) / CLEARED  */
    uint16_t    llid;             /* per-LLID alarms or 0xFFFF*/
    int32_t     reason_code;      /* init/error events        */
    const char *detail;           /* free-form extra info     */
} eponMgr_telemetry_ctx_t;


/* ---- The three producer APIs --------------------------------------- */
int eponMgr_telemetry_raise_simple(eponMgr_telemetry_event_id_t id);

int eponMgr_telemetry_raise_intf  (eponMgr_telemetry_event_id_t id,
                                   const char *ifname);

int eponMgr_telemetry_raise_alarm (eponMgr_telemetry_event_id_t id,
                                   bool raised, uint16_t llid);


/* ---- Lifecycle ----------------------------------------------------- */
int  eponMgr_telemetry_init   (const char *component_name);
int  eponMgr_telemetry_cleanup(void);
bool eponMgr_telemetry_is_enabled(void);
int  eponMgr_telemetry_set_enabled(bool enabled);


/* ---- Harvester (periodic Avro report) ------------------------------ */
int  eponMgr_harvester_init       (uint32_t interval_seconds, bool enabled);
int  eponMgr_harvester_publish_now(eponMgr_data_t *data);
void eponMgr_harvester_cleanup    (void);
```

> **Removed APIs** (obsolete):
> `eponMgr_telemetry_event_type_t`, `eponMgr_telemetry_marker_t`,
> `eponMgr_telemetry_stat_t`, `_report_event`, `_report_onu_status_change`,
> `_report_link_up`, `_report_link_down`, `_report_alarm`, `_report_stats`,
> `_report_single_stat`, `_send_marker`.

---

## 4. New file layout

All processing lives under `src/telemetry/`.
The module is intentionally organized into **four** `.c` files plus one
private header — small enough to navigate, large enough that each file
holds a complete responsibility:

```
include/
  eponMgr_telemetry.h          (public API only — installed)

src/telemetry/
  EponReport.avsc              (Kestrel Avro schema; installed to
                                /usr/ccsp/harvester/EponReport.avsc)
  Makefile.am

  # ---------- Telemetry events (T2 markers) ---------------------------
  eponMgr_telemetry.c          Self-contained event module. Sections:
                                 1. Event descriptor table (34 rows)
                                 2. Value formatter
                                 3. HAL alarm -> event-id mapper
                                 4. T2 backend (t2_event_s shim)
                                 5. Dispatcher + public _raise* APIs

  # ---------- Harvester (periodic Avro report) ------------------------
  eponMgr_harvester.c          Schema load (.avsc once), periodic
                               thread (pthread_cond_timedwait), public
                               init/publish_now/cleanup APIs.

  eponMgr_harvester_avro.c     Avro field population for the three
                               sub-records: CoreHeader (timestamp,
                               UUID, source), CPEIdentifier (gateway
                               MAC, cpe_type), EPONTelemetryData
                               (32 fields from eponMgr_data_t).

  eponMgr_harvester_webpa.c    Frame [MAGIC|UUID|MD5|avro] and ship via
                               libparodus / WebPA. Falls back to a
                               log-only stub when libparodus is absent.

  eponMgr_harvester_priv.h     Private header shared between the three
                               harvester sources. Declares the three
                               `eponMgr_harv_fill_*` and the publish
                               framing entry point. NOT installed.
```

> Rationale for keeping harvester split into 3 files:
> - `_avro.c` is by far the biggest source (≈ 32 field setters); keeping
>   it separate means edits to the Avro schema mapping never touch the
>   thread / scheduling logic.
> - `_webpa.c` carries the libparodus / libcrypto `#ifdef` boundary,
>   isolating optional transports from the rest.
> - `eponMgr_harvester.c` is the orchestration entry point and is the
>   only one rest of the code links to indirectly.

---

## 5. Internal contract (inside `src/telemetry/`)

All internal helpers below are **file-static** — they are not exposed to
any consumer. The split into 4 files is a separation-of-concerns choice,
not an API boundary.

### 5.1 `eponMgr_telemetry.c` (single self-contained module)

Five sections marked by banner comments:

**1. Event descriptor table**

```c
typedef enum { PRIO_INFO, PRIO_WARNING, PRIO_ERROR, PRIO_CRITICAL } priority_t;
typedef enum { FMT_NONE, FMT_INTF, FMT_ALARM } fmt_t;

typedef struct {
    const char *marker;     /* "EPON_ONU_LOS"             */
    priority_t  priority;
    fmt_t       fmt;
} event_desc_t;

static const event_desc_t k_table[EPON_TELEM_EVENT_ID_MAX] = {
    [EPON_TELEM_ONU_LOS]        = { "EPON_ONU_LOS",        PRIO_CRITICAL, FMT_NONE  },
    [EPON_TELEM_INTF_LINK_UP]   = { "EPON_INTF_LINK_UP",   PRIO_INFO,     FMT_INTF  },
    [EPON_TELEM_ALARM_STD_LOFI] = { "EPON_ALARM_STD_LOFI", PRIO_CRITICAL, FMT_ALARM },
    /* … 34 rows total — exact text per Reference §2 … */
};
```

**2. Value formatter**

Pure function `int format_value(desc, ctx, char *out, size_t cap)`:

| `fmt`        | Value template                        |
|--------------|----------------------------------------|
| `FMT_NONE`   | `""`                                   |
| `FMT_INTF`   | `"Interface=%s"`                       |
| `FMT_ALARM`  | `"RAISED"` / `"CLEARED"` (`,LLID=%u` appended when `llid != 0xFFFF`) |

**3. HAL alarm mapper**

```c
static eponMgr_telemetry_event_id_t map_alarm(const epon_alarm_info_t *info);
```
Uses `info->alarm_type` plus the `standard_alarm` / `vendor_alarm` enum.

**4. T2 backend**

```c
static int t2_send(const char *marker, const char *value, priority_t prio);
```
Production: calls `t2_event_s(marker, value)`. Stub: log-only. Built
behind `HAVE_LIBT2`.

**5. Dispatcher + public API**

`dispatch(id, ctx)` does table-lookup → enable check → `format_value` →
`t2_send`. The 3 public `eponMgr_telemetry_raise_*` entry points each
build a `ctx_t` and call `dispatch`.

### 5.2 `eponMgr_harvester.c` (orchestration)

* Owns the schema cache (`avro_schema_t` + `avro_value_iface_t`) and the
  shared writer buffer.
* Owns the periodic thread that does `pthread_cond_timedwait(interval)`
  then calls the local `build_and_ship()`.
* `eponMgr_harvester_publish_now()` is also explicitly callable from the
  stats poller after a successful `collect_all_stats()`.

### 5.3 `eponMgr_harvester_avro.c` (Avro encoding)

Three entry points populate one Avro sub-record each:

```c
int eponMgr_harv_fill_header(avro_value_t *report); /* CoreHeader      */
int eponMgr_harv_fill_cpe   (avro_value_t *report); /* CPEIdentifier   */
int eponMgr_harv_fill_data  (avro_value_t *report,  /* EPONTelemetry-  */
                              eponMgr_data_t *data); /* Data, 32 fields */
```

Float optical/transceiver values from the HAL are converted to the
Dbm1000 / mC / mV / uA integer encoding by multiplication by 1000.
Source mapping is detailed in
[EPON_Manager_Reference_v2.md §3.1.2–3.1.4](EPON_Manager_Reference_v2.md#312-link-statistics).

### 5.4 `eponMgr_harvester_webpa.c` (transport)

```c
int eponMgr_harv_publish_frame(uint8_t *frame, size_t total_len);
```

Fills the schema-id prefix `[MAGIC(0x8A) | UUID(16B) | MD5(16B)]` and
ships the full frame via `libparodus_send()`. Falls back to a logging
stub when `libparodus` is not available so the rest of the pipeline
remains exercisable in tests.

---

## 6. Integration points (caller changes)

| Location | Old | New |
|----------|-----|-----|
| `src/core/epon_manager_main.c` after `init` ok | — | `eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_INIT_SUCCESS)` |
| `epon_manager_main.c` init failure | log only | `eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_INIT_FAILURE)` |
| `epon_manager_main.c` SIGTERM | log only | `eponMgr_telemetry_raise_simple(EPON_TELEM_SYSTEM_SHUTDOWN)` |
| `controller.c::process_onu_status_event` | `// TODO Phase 7` | `EPON_TELEM_ONU_LOS / DOWNSTREAM_SIGNAL_DETECTED / REGISTRATION / DEREGISTRATION` (one of) |
| `controller.c::process_interface_status_event` | `// TODO Phase 7` | `_raise_intf(EPON_TELEM_INTF_LINK_UP/DOWN)` then `_raise_simple(PHY_STATUS_UP/DOWN)` based on aggregate |
| `controller.c::process_alarm_event` | log only | `id = telem_alarm_to_id(info); _raise_alarm(id, info->is_active, info->llid)` |
| HAL-call error sites | log only | `EPON_TELEM_ERROR_HAL_CALL_FAILED` |
| `queue.c` push fail | log only | `EPON_TELEM_ERROR_EVENT_QUEUE_FULL` |
| `stats_poller.c` collect failure | log only | `EPON_TELEM_ERROR_STATS_COLLECTION_FAILED` |
| `rbus.c` publish failure | log only | `EPON_TELEM_ERROR_RBUS_PUBLISH_FAILED` |
| `psm.c` get/set fail | log only | `EPON_TELEM_ERROR_PSM_ACCESS_FAILED` |
| `tr181.c` `Reset` SET handler | log only | `EPON_TELEM_SYSTEM_ONU_RESET` |
| `tr181.c` `FactoryReset` SET handler | log only | `EPON_TELEM_SYSTEM_FACTORY_RESET` |
| `stats_poller.c` after `collect_all_stats()` | `// TODO push to telemetry` | `eponMgr_harvester_publish_now(poller->eponData)` |

No marker strings, no value formatting, no severity logic anywhere outside
`src/telemetry/`.

---

## 7. Build-system changes

`src/telemetry/Makefile.am`:

```make
noinst_LIBRARIES = libepon_telemetry.a

libepon_telemetry_a_SOURCES = \
    eponMgr_telemetry.c            \
    eponMgr_harvester.c            \
    eponMgr_harvester_avro.c       \
    eponMgr_harvester_webpa.c

libepon_telemetry_a_CFLAGS = \
    -I$(top_srcdir)/include        \
    -I$(top_srcdir)/src/logger     \
    -I$(top_srcdir)/src/core/data_structures \
    -I${PKG_CONFIG_SYSROOT_DIR}$(includedir)/epon \
    $(AVRO_CFLAGS) $(AM_CFLAGS)

harvesterdir   = $(prefix)/ccsp/harvester
harvester_DATA = EponReport.avsc
```

`configure.ac`:
* `PKG_CHECK_MODULES([AVRO], [avro-c])`
* Detect `libtelemetry_msgsender` (T2 — `HAVE_LIBT2`)
* Detect `libparodus` (WebPA — `HAVE_LIBPARODUS`)
* Detect OpenSSL `libcrypto` for schema MD5 (`HAVE_LIBCRYPTO`)
* `epon_manager_LDADD += $(AVRO_LIBS) $(T2_LIBS) $(PARODUS_LIBS) $(CRYPTO_LIBS)`

---

## 8. Implementation order (execution checklist)

1. **Header refactor** — rewrite `include/eponMgr_telemetry.h` to the API in §3.
2. **Event module** — `eponMgr_telemetry.c` end-to-end (table, formatter,
   alarm mapper, T2 backend, dispatcher, public APIs) in one file with
   five clearly-banner-commented sections.
3. **Wire all caller sites** in `src/core/` and `src/rbus/` (table in §6).
4. **Harvester orchestration** — `eponMgr_harvester.c` (schema cache,
   periodic thread, `init/publish_now/cleanup`).
5. **Avro encoder** — `eponMgr_harvester_avro.c` (header, cpe, data —
   32 fields).
6. **WebPA publisher** — `eponMgr_harvester_webpa.c`
   (`[MAGIC|UUID|MD5|avro]` + libparodus).
7. **Wire stats poller** to call `eponMgr_harvester_publish_now()`.
8. **Update `Makefile.am` and `configure.ac`** for `avro-c`,
   `libtelemetry_msgsender`, `libparodus`, `libcrypto`.
9. **Bench test with `tests/hal_mock`**: trigger every event id at least
   once; force a stats collection; verify a single Avro record is
   logged/sent per interval.

Each step is independently buildable; no step leaves the tree in a
non-compiling state.

---

## 9. Acceptance criteria

* Every event id in `eponMgr_telemetry_event_id_t` is reachable from at least
  one production code path.
* No marker name string literal exists outside `src/telemetry/`.
* No `avro_*` symbol is referenced outside `src/telemetry/`.
* `EPONTelemetryDiagnostics` Avro record validates against `EponReport.avsc`.
* `tests/hal_mock` exercise produces:
  - 34 event ids fired (verified via T2 stub log),
  - 1 Avro report per `PollingInterval` (verified via publisher log).
* `make check` passes; no new compiler warnings.

---

## 10. Out of scope (explicitly)

* Migration of historical per-stat T2 markers (replaced by the harvester report — see [Reference §2.8 note](EPON_Manager_Reference_v2.md#28-summary-statistics)).
* Telemetry rate-limiting / dedup beyond what T2 already provides.
* Webpa transport details — re-uses existing CCSP `sendWebpaMsg()`.
* `Device.Optical.Interface.{i}.X_RDK_EPON.StatsPoller.Enable=false` short-circuiting harvester (handled trivially by `_publish_now()` early-return).
