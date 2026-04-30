# EPON Manager — Telemetry & Harvester Module Design

**Version:** 1.0  |  **Date:** April 30, 2026
**Companion:** [07_Telemetry_Implementation_Plan.md](07_Telemetry_Implementation_Plan.md)
**Spec:** [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) §2 / §3

---

## 1. Design goals

1. **Single producer surface** — the rest of the code base only knows
   *event ids*. Marker names, severity, value formatting, T2 dispatch,
   and Avro encoding are all hidden inside `src/telemetry/`.
2. **Two independent sub-pipelines** sharing one module boundary:
   - **Event pipeline** — discrete markers via T2 (`t2_event_s`).
   - **Harvester pipeline** — periodic `EPONTelemetryDiagnostics` Avro
     record over CCSP/WebPA, modeled on `rdk-xdslmanager`.
3. **Replaceable backends** — T2 stub vs. real `libtelemetry_msgsender`,
   and harvester stub vs. real `sendWebpaMsg` are guarded at link/compile
   time only. Caller code never changes.

---

## 2. Module boundary

```mermaid
flowchart LR
    subgraph caller [Rest of EPON Manager]
        MAIN[epon_manager_main.c]
        CTRL[controller.c]
        SP[stats_poller.c]
        RB[rbus / tr181 / psm]
    end

    subgraph telem [src/telemetry/  &mdash; sole owner of telemetry logic]
        API[[eponMgr_telemetry.h<br/>3 producer APIs + harvester API]]
        TELEM(eponMgr_telemetry.c<br/>table + format + alarm-map<br/>+ T2 backend + dispatcher)
        HARV(eponMgr_harvester.c<br/>schema + thread + orchestration)
        HAVRO(eponMgr_harvester_avro.c<br/>header / cpe / data fields)
        HWPA(eponMgr_harvester_webpa.c<br/>frame + libparodus)
    end

    subgraph ext [External]
        T2[T2 daemon<br/>libtelemetry_msgsender]
        WEBPA[CCSP / WebPA &rarr; Harvester]
    end

    MAIN --> API
    CTRL --> API
    SP   --> API
    RB   --> API

    API --> TELEM
    TELEM --> T2

    API --> HARV
    HARV --> HAVRO
    HARV --> HWPA
    HWPA --> WEBPA
```

> Only `eponMgr_telemetry.h` crosses the dashed boundary.

---

## 3. File responsibilities

The telemetry module is implemented in **4 `.c` files + 1 private header**
— each file owns a complete, self-contained responsibility.

| File | Owns |
|------|------|
| `eponMgr_telemetry.h` | Public surface: 34 event ids, 3 producer APIs (`_raise_simple`, `_raise_intf`, `_raise_alarm`) and 3 harvester APIs. Only header callers ever include. |
| `eponMgr_telemetry.c` | Self-contained event module. Five sections inside the file: (1) const event descriptor table, (2) value formatter (`Interface=…`, `RAISED`/`CLEARED[,LLID=N]`), (3) HAL alarm → event-id mapper, (4) T2 backend (`t2_event_s` shim, log-only when `HAVE_LIBT2` undefined), (5) dispatcher + public init/cleanup/raise. All internals are file-static. |
| `eponMgr_harvester.c` | Schema load (`.avsc` once), periodic thread (`pthread_cond_timedwait`), `build_and_ship` orchestration, public `eponMgr_harvester_init/_publish_now/_cleanup`. |
| `eponMgr_harvester_avro.c` | Avro encoding for the three sub-records: Kestrel `CoreHeader` (timestamp, uuid v4, source), `CPEIdentifier` (gateway MAC, cpe_type), and `EPONTelemetryData` (32 fields from `eponMgr_data_t`, including float→Dbm1000 conversion). |
| `eponMgr_harvester_webpa.c` | Frames `[MAGIC \| UUID \| MD5 \| avro]` and ships via `libparodus`. Carries the optional `HAVE_LIBPARODUS` and `HAVE_LIBCRYPTO` `#ifdef` boundaries. Falls back to a logging stub when libparodus is absent. |
| `eponMgr_harvester_priv.h` | Private header shared by the three harvester sources. Declares `eponMgr_harv_fill_header/_cpe/_data` and `eponMgr_harv_publish_frame`, plus framing constants. NOT installed. |

---

## 4. Event raise — sequence

```mermaid
sequenceDiagram
    autonumber
    participant Ctrl as controller.c
    participant API as eponMgr_telemetry.h
    participant Telem as eponMgr_telemetry.c
    participant T2D as T2 daemon

    Ctrl->>API: raise_intf(EPON_TELEM_INTF_LINK_UP, "veip0")
    API->>Telem: _raise(id, &ctx{ifname="veip0"})
    Note over Telem: lookup(id) -> { "EPON_INTF_LINK_UP",<br/>INFO, FMT_INTF }<br/>format -> "Interface=veip0"
    Telem->>T2D: t2_event_s("EPON_INTF_LINK_UP",<br/>"Interface=veip0")
    Telem-->>API: 0
    API-->>Ctrl: 0
```

---

## 5. Alarm raise — sequence (RAISED / CLEARED)

```mermaid
sequenceDiagram
    autonumber
    participant Ctrl as controller.c
    participant API as eponMgr_telemetry.h
    participant Telem as eponMgr_telemetry.c
    participant T2 as T2 daemon

    Note over Ctrl: HAL fired epon_alarm_info_t<br/>(STANDARD, LOFI, is_active=true,<br/>llid=0xFFFF)
    Ctrl->>API: raise_alarm(&info)
    API->>Telem: _raise_alarm(info)
    Note over Telem: map_alarm(info) ->
    Note over Telem: EPON_TELEM_ALARM_STD_LOFI<br/>format -> "RAISED"
    Telem->>T2: t2_event_s("EPON_ALARM_STD_LOFI",<br/>"RAISED")
```

When the alarm clears, the same path runs with `info->is_active=false`
→ value `"CLEARED"`.

---

## 6. Harvester pipeline — periodic publish

```mermaid
sequenceDiagram
    autonumber
    participant SP as stats_poller.c
    participant DATA as eponMgr_data_t
    participant Harv as harvester.c
    participant Avro as harvester_avro.c
    participant Pub as harvester_webpa.c
    participant W as CCSP/WebPA

    Note over SP: every PollingInterval (default 900 s)
    SP->>DATA: collect_all_stats()
    SP->>Harv: harvester_publish_now(data)
    Harv->>Avro: fill_header + fill_cpe + fill_data
    Avro-->>Harv: avro_value populated
    Note over Harv: avro_value_write -> avro_buf
    Harv->>Pub: publish_frame(avro_buf, size)
    Note right of Pub: frame = MAGIC(0x8a) + UUID(16B)<br/>+ MD5(16B) + avro
    Pub->>W: libparodus_send(... wrp_msg ...)
    W-->>Pub: ok
    Pub-->>Harv: 0
    Harv-->>SP: 0
```

---

## 7. Threading model

```mermaid
flowchart TB
    subgraph proc [rdk-eponmanager process]
        MT(main thread)
        EVT(event-processor thread<br/>controller.c)
        STP(stats-poller thread<br/>stats_poller.c)
        HRT(harvester thread<br/>eponMgr_harvester.c)
    end

    EVT -- raise_simple/intf/alarm --> TELE([eponMgr_telemetry.c<br/>mutex-protected state])
    STP -- raise_*/publish_now --> TELE
    MT  -- raise_simple --> TELE
    HRT -- internal publish --> TELE

    TELE -- t2_event_s --> T2(T2 daemon)
    TELE -- libparodus_send --> WP(CCSP bus)
```

* Producer side is fully thread-safe — internal mutex in `eponMgr_telemetry.c`.
* Harvester thread owns its own `pthread_cond_t` for timed wait + early
  wakeup on shutdown / interval change.
* `harvester_publish_now()` may be invoked from any thread (stats poller
  or harvester thread itself); serialization handled inside the publisher.

---

## 8. Data flow inside the harvester report

Mapping is locked to [Reference §3.1.2–3.1.4](EPON_Manager_Reference_v2.md#312-link-statistics).

```mermaid
flowchart LR
    HAL1[epon_hal_get_link_stats] --> DATA1[eponMgr_data_t.link_stats]
    HAL2[epon_hal_get_transceiver_stats] --> DATA2[eponMgr_data_t.transceiver_stats]
    HAL3[epon_hal_get_onu_status] --> DATA3[eponMgr_data_t.onu_state]

    DATA1 --> AVRO[EPONTelemetryData record]
    DATA2 --> AVRO
    DATA3 --> AVRO

    HEADER[CoreHeader<br/>+ CPEIdentifier] --> RECORD[EPONTelemetryDiagnostics]
    AVRO --> RECORD

    RECORD --> SER[avro_writer_memory<br/>binary serialize]
    SER --> FRAME[MAGIC + UUID + MD5 + bin]
    FRAME --> WP[libparodus_send]
```

Field correspondence (32 Avro fields ← TR-181 / HAL):

| Avro field | Source struct field |
|------------|---------------------|
| `BytesSent / BytesReceived / PacketsSent / …` | `epon_hal_link_stats_t` |
| `FECCorrected / FECUncorrectable / RangingResyncs / MACResets` | `epon_hal_link_stats_t` (extensions) |
| `BER` | derived in stats poller |
| `TransmitOpticalLevel / OpticalSignalLevel` | `epon_hal_transceiver_stats_t` |
| `LowerOpticalThreshold / UpperOpticalThreshold / Lower/UpperTransmitPowerThreshold` | `epon_hal_transceiver_stats_t` |
| `TransceiverTemperature / SupplyVoltage / LaserBiasCurrent` | `epon_hal_transceiver_stats_t` |
| `OperationalMode / EncryptionMode / ONUStatus` | `eponMgr_onu_state_t` |
| `MaxBitRate` | `eponMgr_data_t.max_bit_rate` |

---

## 9. State machine — alarm raised/cleared encoding

```mermaid
stateDiagram-v2
    [*] --> Cleared
    Cleared --> Raised: HAL alarm_callback(is_active=true)<br/>raise_alarm(id, true, llid)<br/>value="RAISED"
    Raised  --> Cleared: HAL alarm_callback(is_active=false)<br/>raise_alarm(id, false, llid)<br/>value="CLEARED"
```

The state itself is owned by HAL; the telemetry module is **stateless**
with respect to alarms — it forwards each transition as one T2 event.

---

## 10. Error & resource handling

| Failure | Behaviour |
|---------|-----------|
| `_raise*` called before `_init` | Return `-1`; no log spam (rate-limited). |
| `_raise*` with unknown id | Return `-1`; log once. |
| T2 backend send error | Logged at `WARN`; producer return value unchanged (success), so callers don't cascade-fail on telemetry hiccups. |
| Avro encode error in harvester | Whole report skipped; `EPON_TELEM_ERROR_HAL_CALL_FAILED` event raised (avoids reporting infinite recursion via guard flag). |
| `libparodus_send` error | Logged at `WARN`; record dropped; next interval retries. |
| Schema file missing | `harvester_init` returns `-1`; harvester thread does not start; producer events still work. |

---

## 11. Test plan (mapped to acceptance criteria §9 of plan)

```mermaid
flowchart LR
    HAL_MOCK[tests/hal_mock<br/>epon_hal_trigger.c] --> CB1(status_callback)
    HAL_MOCK --> CB2(alarm_callback)
    HAL_MOCK --> CB3(interface_status_callback)
    CB1 --> CTRL2[controller.c]
    CB2 --> CTRL2
    CB3 --> CTRL2
    CTRL2 --> TELE2[telemetry::raise_*]
    TELE2 --> T2STUB[T2 stub log]
    HAL_MOCK -- timer --> SP2[stats_poller.c]
    SP2 --> HARV2[harvester::publish_now]
    HARV2 --> AVRO2[avro stub log]

    T2STUB -.assert.-> ASSERT[34 ids fired]
    AVRO2  -.assert.-> ASSERT2[1 record per interval]
```

Test driver iterates the trigger harness through every code path and
verifies the two assertions.

---

## 12. Reference

* [EPON_Manager_Reference_v2.md](EPON_Manager_Reference_v2.md) — TR-181, telemetry, harvester spec.
* `rdkxdslmanager/source/TR-181/integration_src.shared/xdsl_report.{c,h}` — reference implementation of the harvester pattern (Avro + WebPA + magic-byte framing).
* Apache Avro 1.11 C API.
* IEEE 802.3ah Clause 57 (OAM) — alarm taxonomy.
* RDK T2 Telemetry Framework — marker semantics.
