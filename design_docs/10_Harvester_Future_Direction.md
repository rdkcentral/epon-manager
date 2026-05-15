# EPON Manager — Harvester Future Direction

**Version:** 1.0  |  **Date:** May 15, 2026
**Status:** Deferred — not part of current telemetry implementation
**Prerequisite:** Telemetry markers implemented per [07_Telemetry_Implementation_Plan.md](07_Telemetry_Implementation_Plan.md)

---

## 1. Purpose

This document captures the design direction for a future **Harvester periodic
Avro report** (`EPONTelemetryDiagnostics`) that consolidates EPON link
statistics, transceiver diagnostics, and ONU status into a single binary
record published to the Kestrel analytics pipeline.

The harvester was originally planned as part of the initial telemetry story
but has been deferred. This document preserves the architectural decisions,
schema design, and implementation guidance so a future developer can pick
up the work without re-deriving the design.

---

## 2. Overview

| Aspect | Detail |
|--------|--------|
| **Report name** | `EPONTelemetryDiagnostics` |
| **Schema namespace** | `com.comcast.kestrel.odp.event` |
| **Transport** | Binary Avro over CCSP/WebPA via `libparodus` |
| **Frame format** | `[MAGIC(0x8A) | UUID(16B) | SCHEMA_MD5(16B) | avro_binary]` |
| **Reference implementation** | `rdkxdslmanager/source/TR-181/integration_src.shared/xdsl_report.c` |
| **Cadence** | One report per `StatsPoller.PollingInterval` (default 900 s) |
| **Data source** | `eponMgr_data_t` populated by the stats poller |

---

## 3. Proposed architecture

### 3.1 New files (under `src/telemetry/`)

| File | Responsibility |
|------|---------------|
| `eponMgr_harvester.c` | Schema load (`.avsc` once), periodic thread (`pthread_cond_timedwait`), `build_and_ship` orchestration, public `init/publish_now/cleanup` API. |
| `eponMgr_harvester_avro.c` | Avro encoding for the three sub-records: `CoreHeader` (timestamp, UUID v4, source), `CPEIdentifier` (gateway MAC, cpe\_type), `EPONTelemetryData` (32 fields from `eponMgr_data_t`). |
| `eponMgr_harvester_webpa.c` | Frame `[MAGIC\|UUID\|MD5\|avro]` and ship via `libparodus`. Falls back to a logging stub when libparodus is absent. |
| `eponMgr_harvester_priv.h` | Private header shared between the three harvester sources. NOT installed. |
| `EponReport.avsc` | Kestrel Avro schema. Installed to `/usr/ccsp/harvester/EponReport.avsc`. |

### 3.2 Public API additions to `eponMgr_telemetry.h`

```c
/* Harvester (periodic Avro report) */
int  eponMgr_harvester_init       (uint32_t interval_seconds, bool enabled);
int  eponMgr_harvester_publish_now(eponMgr_data_t *data);
void eponMgr_harvester_cleanup    (void);
```

### 3.3 Module boundary

```
caller code
    │
    ├── raise_simple / raise_intf / raise_alarm   ──► eponMgr_telemetry.c ──► T2
    │
    └── harvester_publish_now(data)               ──► eponMgr_harvester.c
                                                         ├── harvester_avro.c (encode)
                                                         └── harvester_webpa.c (frame+ship)
                                                                  └── libparodus ──► Kestrel
```

---

## 4. Additional build dependencies

| Library | Purpose | Optional? |
|---------|---------|-----------|
| `libavro-c` | Avro binary encoding | Required |
| `libparodus` | WebPA transport to Kestrel | Optional (stub fallback) |
| `libcrypto` (OpenSSL) | MD5 of schema file | Optional (zeros fallback) |

`configure.ac` additions:
```
PKG_CHECK_MODULES([AVRO], [avro-c])
AC_CHECK_LIB([parodus], [libparodus_init], [...])
AC_CHECK_LIB([crypto], [MD5_Init], [...])
```

BitBake (`rdk-eponmanager.bbappend`):
```
DEPENDS_append = " avro-c libparodus openssl"
RDEPENDS_${PN}_append = " avro-c libparodus"
LDFLAGS_append = " -lavro -llibparodus -lcrypto"
FILES_${PN}_append = " /usr/ccsp/harvester/EponReport.avsc"
```

---

## 5. Avro schema

The `EPONTelemetryDiagnostics` schema contains 32 data fields organised
into three sub-records:

- **CoreHeader** — `timestamp` (timestamp-millis), `uuid` (fixed[16]), `source`
- **CPEIdentifier** — `mac_address` (fixed[6]), `cpe_type`
- **EPONTelemetryData** — 20 link-statistics fields, 9 transceiver fields,
  3 ONU-status fields

All data fields are `union[null, T]` to handle unavailable values.
Float optical/transceiver values from the HAL must be converted to
×1000 fixed-point integer encoding (Dbm1000 / m°C / mV / µA).

The complete schema definition and field mappings were previously captured
in [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) Section 3
(now removed from that document). The full `EponReport.avsc` JSON and
field-to-HAL-struct mapping should be restored from the git history of
this repository (branch `feature/telemetry-docs`, commits before
the harvester removal).

---

## 6. Data flow

```
epon_hal_get_link_stats()        ──► eponMgr_data_t.link_stats
epon_hal_get_transceiver_stats() ──► eponMgr_data_t.transceiver_stats
epon_hal_get_onu_status()        ──► eponMgr_data_t.onu_state
                                          │
                                          ▼
                                   harvester_avro.c
                                   fill_header + fill_cpe + fill_data
                                          │
                                          ▼
                                   avro_value_write → binary buffer
                                          │
                                          ▼
                                   harvester_webpa.c
                                   frame: MAGIC + UUID + MD5 + avro
                                          │
                                          ▼
                                   libparodus_send → Kestrel
```

---

## 7. Integration points

| Location | Change |
|----------|--------|
| `epon_manager_main.c` | Call `eponMgr_harvester_init(interval, enabled)` after telemetry init; call `eponMgr_harvester_cleanup()` on shutdown. |
| `eponMgr_stats_poller.c` | After successful `collect_all_stats()`, call `eponMgr_harvester_publish_now(data)`. |
| `src/telemetry/Makefile.am` | Add harvester `.c` files to `SOURCES`; add `$(AVRO_CFLAGS)` to `CFLAGS`; install `EponReport.avsc`. |
| `src/core/Makefile.am` | Add `$(AVRO_LIBS) $(PARODUS_LIBS) $(CRYPTO_LIBS) -lm` to `LDADD`. |

---

## 8. Threading considerations

- The harvester thread should use `pthread_cond_timedwait` (not `sleep`)
  for its inter-report wait, enabling prompt shutdown.
- `harvester_publish_now()` may be invoked from the stats-poller thread
  or the harvester's own timer thread; a `pthread_mutex_t` must serialize
  the shared Avro writer buffer.
- T2 event delivery and harvester publishing should remain independently
  operable — a failure in one must not silence the other.

---

## 9. Acceptance criteria (when implemented)

- Exactly one Avro record per `PollingInterval` observed on the publisher log.
- Captured bytes validate against `EponReport.avsc` via `avro-tools`.
- No `avro_*` or `libparodus_*` symbol referenced outside `src/telemetry/`.
- `StatsPoller.Enable=false` blocks harvester publishes.
- Schema MD5 computed once at startup and reused.

---

## 10. References

- `rdkxdslmanager/source/TR-181/integration_src.shared/xdsl_report.{c,h}` — reference harvester pattern
- Apache Avro 1.11 C API
- Kestrel pipeline documentation
- Git history of this repository (`feature/telemetry-docs` branch) for the
  original `EponReport.avsc`, `eponMgr_harvester*.c`, and
  `eponMgr_harvester_priv.h` implementations
